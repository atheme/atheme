/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 Marien Zwart
 * Copyright (C) 2018, 2021 Atheme Development Group (https://atheme.github.io/)
 *
 * Canonicalize multiple email domains to treat them as one.
 */

#include <atheme.h>

static mowgli_patricia_t *canon_domain_map;

static void
email_canonicalize_domains(char email[static (EMAILLEN + 1)],
                           void ATHEME_VATTR_UNUSED *const restrict user_data)
{
	char *domain = strchr(email, '@');

	if (!domain)
		return;

	// skip the actual @ itself
	domain++;

	const char *canon_domain = mowgli_patricia_retrieve(canon_domain_map, domain);

	if (!canon_domain)
		return;

	// this should never fail as canon domains should never be
	// greater in length than their input domains;
	// double-check anyway to ensure strcpy won't overflow
	return_if_fail((domain - email + strlen(canon_domain)) <= EMAILLEN);

	slog(LG_DEBUG, "%s: '%s' -> '%s' for address '%s'", MOWGLI_FUNC_NAME, domain, canon_domain, email);
	strcpy(domain, canon_domain);
}

static int
c_canon_domains(mowgli_config_file_entry_t *ce)
{
	if (ce->entries == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	mowgli_config_file_entry_t *group;

	MOWGLI_ITER_FOREACH(group, ce->entries)
	{
		if (strcasecmp("PROVIDER", group->varname) != 0)
		{
			conf_report_warning(group, "Invalid configuration option");
			continue;
		}

		if (group->entries == NULL)
		{
			conf_report_warning(group, "no parameter for configuration option");
			continue;
		}

		mowgli_config_file_entry_t *domain_ce;

		const char *shortest = NULL;
		size_t shortest_len = SIZE_MAX;

		// iterate once over all entries to find the shortest entry
		MOWGLI_ITER_FOREACH(domain_ce, group->entries)
		{
			// We need to exclude duplicates from the shortest entry calculation.
			// Don't report a warning as we'll do so during the second loop
			if (mowgli_patricia_retrieve(canon_domain_map, domain_ce->varname))
				continue;

			size_t len = strlen(domain_ce->varname);
			if (len < shortest_len)
			{
				shortest = domain_ce->varname;
				shortest_len = len;
			}
		}

		// iterate again, this time adding entries mapping them to the shortest entry
		MOWGLI_ITER_FOREACH(domain_ce, group->entries)
		{
			// Check for duplicates again. We could avoid this step by creating a new
			// list without duplicates in the first loop; however, we'd then also have
			// to check for entries that are duplicated within the same provider's list
			// separately, negating any real benefit of that.
			if (mowgli_patricia_retrieve(canon_domain_map, domain_ce->varname))
			{
				conf_report_warning(group, "duplicate entry for domain");
				continue;
			}

			if (!shortest)
			{
				slog(LG_ERROR, "%s: passed duplicate check but no shortest domain found (BUG)", MOWGLI_FUNC_NAME);
				break;
			}

			mowgli_patricia_add(canon_domain_map, domain_ce->varname, sstrdup(shortest));
		}
	}

	return 0;
}

static void canon_domain_map_destroy_cb(const char ATHEME_VATTR_UNUSED *key, void *data, void ATHEME_VATTR_UNUSED *privdata)
{
	sfree(data);
}

static void
on_config_purge(void ATHEME_VATTR_UNUSED *hdata)
{
	mowgli_patricia_destroy(canon_domain_map, &canon_domain_map_destroy_cb, NULL);
	canon_domain_map = mowgli_patricia_create(&strcasecanon);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	canon_domain_map = mowgli_patricia_create(&strcasecanon);

	hook_add_config_purge(&on_config_purge);
	add_top_conf("CANON_DOMAINS", &c_canon_domains);
	register_email_canonicalizer(&email_canonicalize_domains, NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	unregister_email_canonicalizer(&email_canonicalize_domains, NULL);
	hook_del_config_purge(&on_config_purge);
	del_top_conf("CANON_DOMAINS");

	mowgli_patricia_destroy(canon_domain_map, &canon_domain_map_destroy_cb, NULL);
}

SIMPLE_DECLARE_MODULE_V1("misc/canon_domains", MODULE_UNLOAD_CAPABILITY_OK)
