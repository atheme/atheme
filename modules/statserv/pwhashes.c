/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

#include "pwhashes.h"

static void
ss_cmd_pwhashes_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                     char ATHEME_VATTR_UNUSED **const restrict parv)
{
	unsigned int pwhashtypes[PWHASH_TYPE_TOTAL_COUNT];

	(void) logcommand(si, CMDLOG_GET, "PWHASHES");
	(void) memset(&pwhashtypes, 0x00, sizeof pwhashtypes);

	struct myentity *mt;
	struct myentity_iteration_state state;
	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		const enum pwhash_type type = pwhash_get_type(user(mt));

		pwhashtypes[type]++;
	}

	(void) command_success_nodata(si, "%24s %16s %8s", _("Module Name"), _("Algorithm"), _("Count"));
	(void) command_success_nodata(si, "%s %s %s", "------------------------", "----------------", "--------");

	for (enum pwhash_type i = PWHASH_TYPE_NONE; i < PWHASH_TYPE_TOTAL_COUNT; i++)
	{
		if (! pwhashtypes[i])
			continue;

		(void) command_success_nodata(si, "%24s %16s %8u", pwhash_type_to_modname[i],
		                                  pwhash_type_to_algorithm[i], pwhashtypes[i]);
	}

	(void) command_success_nodata(si, _("End of output."));
}

static struct command ss_cmd_pwhashes = {
	.name           = "PWHASHES",
	.desc           = N_("Shows database password hash statistics."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &ss_cmd_pwhashes_func,
	.help           = { .path = "statserv/pwhashes" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "statserv/main")

	(void) service_named_bind_command("statserv", &ss_cmd_pwhashes);
	(void) hook_add_myuser_changed_password_or_hash(&pwhash_invalidate_user_token);

	// If this module was unloaded instead of reloaded, the cache could be stale
	(void) pwhash_invalidate_token_cache();
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("statserv", &ss_cmd_pwhashes);
	(void) hook_del_myuser_changed_password_or_hash(&pwhash_invalidate_user_token);
}

SIMPLE_DECLARE_MODULE_V1("statserv/pwhashes", MODULE_UNLOAD_CAPABILITY_OK)
