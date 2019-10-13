/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * search.c - rpgserv SEARCH command
 */

#include <atheme.h>
#include "prettyprint.h"

static void
rs_cmd_search(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_patricia_iteration_state_t state;
	struct mychan *mc;
	unsigned int listed = 0;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		unsigned int i, j;
		struct metadata *md;

		static const char *mdkeys[] = {
			"private:rpgserv:genre", "private:rpgserv:period", "private:rpgserv:ruleset",
			"private:rpgserv:rating", "private:rpgserv:system"
		};

		if (!mc->chan)
			continue;
		if (CMODE_SEC & mc->chan->modes || CMODE_PRIV & mc->chan->modes)
			continue;

		if (!metadata_find(mc, "private:rpgserv:enabled"))
			continue;

		for (i = 0; i < ARRAY_SIZE(mdkeys); i++)
		{
			md = metadata_find(mc, mdkeys[i]);
			if (md == NULL)
				continue;

			for (j = 0; j < (unsigned int) parc; j++)
			{
				if (strcasestr(md->value, parv[j]) != NULL)
					goto __matched;
			}
		}

		continue;

__matched:
		listed++;

		command_success_nodata(si, _("Channel \2%s\2:"), mc->name);
		md = metadata_find(mc, "private:rpgserv:genre");
		command_success_nodata(si, _("Genre: %s"), rs_prettyprint_keywords(md, genre_keys, genre_names, ARRAY_SIZE(genre_keys)));
		md = metadata_find(mc, "private:rpgserv:period");
		command_success_nodata(si, _("Period: %s"), rs_prettyprint_keywords(md, period_keys, period_names, ARRAY_SIZE(period_keys)));
		md = metadata_find(mc, "private:rpgserv:ruleset");
		command_success_nodata(si, _("Ruleset: %s"), rs_prettyprint_keywords(md, ruleset_keys, ruleset_names, ARRAY_SIZE(ruleset_keys)));
		md = metadata_find(mc, "private:rpgserv:rating");
		command_success_nodata(si, _("Rating: %s"), rs_prettyprint_keywords(md, rating_keys, rating_names, ARRAY_SIZE(rating_keys)));
		md = metadata_find(mc, "private:rpgserv:system");
		command_success_nodata(si, _("System: %s"), rs_prettyprint_keywords(md, system_keys, system_names, ARRAY_SIZE(system_keys)));
	}

	command_success_nodata(si, ngettext(N_("\2%u\2 channel met your criteria."),
	                                    N_("\2%u\2 channels met your criteria."),
	                                    listed), listed);

	command_success_nodata(si, _("For more information on a specific channel, use \2/msg %s INFO <channel>\2."), si->service->disp);

	logcommand(si, CMDLOG_GET, "RPGSERV:SEARCH");
}

static struct command rs_search = {
	.name           = "SEARCH",
	.desc           = N_("Search for games based on specific criteria."),
	.access         = AC_NONE,
	.maxparc        = 20,
	.cmd            = &rs_cmd_search,
	.help           = { .path = "rpgserv/search" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "rpgserv/main")

	service_named_bind_command("rpgserv", &rs_search);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("rpgserv", &rs_search);
}

SIMPLE_DECLARE_MODULE_V1("rpgserv/search", MODULE_UNLOAD_CAPABILITY_OK)
