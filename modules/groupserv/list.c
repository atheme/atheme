/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static const struct groupserv_core_symbols *gcsyms = NULL;

// Perhaps add criteria to groupser/list like there is now in chanserv/list and nickserv/list in the future
static void
gs_cmd_list_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LIST");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: LIST <group pattern>"));
		return;
	}

	const char *const param = parv[0];

	// No need to say "Groups currently registered". You can't have a unregistered group.
	(void) command_success_nodata(si, _("Groups matching pattern \2%s\2:"), param);

	struct myentity *mt;
	struct myentity_iteration_state state;
	unsigned int matches = 0;

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		struct mygroup *mg = group(mt);
		continue_if_fail(mt != NULL);
		continue_if_fail(mg != NULL);

		if (match(param, entity(mg)->name) == 0)
		{
			(void) command_success_nodata(si, _("- %s (%s)"), entity(mg)->name,
			                              gcsyms->mygroup_founder_names(mg));
			matches++;
		}
	}

	if (! matches)
		(void) command_success_nodata(si, _("No groups matched pattern \2%s\2"), param);
	else
		(void) command_success_nodata(si, ngettext(N_("\2%d\2 match for pattern \2%s\2"),
		                              N_("\2%d\2 matches for pattern \2%s\2"), matches), matches, param);

	(void) logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%d\2 matches)", param, matches);
}

static struct command gs_cmd_list = {
	.name           = "LIST",
	.desc           = N_("List registered groups."),
	.access         = PRIV_GROUP_AUSPEX,
	.maxparc        = 1,
	.cmd            = &gs_cmd_list_func,
	.help           = { .path = "groupserv/list" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_list);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_list);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/list", MODULE_UNLOAD_CAPABILITY_OK)
