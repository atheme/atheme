/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include <atheme.h>
#include "groupserv.h"

// Perhaps add criteria to groupser/list like there is now in chanserv/list and nickserv/list in the future
static void
gs_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct myentity *mt;
	char *pattern = parv[0];
	unsigned int matches = 0;
	struct myentity_iteration_state state;

	if (!pattern)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LIST");
		command_fail(si, fault_needmoreparams, _("Syntax: LIST <group pattern>"));
		return;
	}

	// No need to say "Groups currently registered". You can't have a unregistered group.
	command_success_nodata(si, _("Groups matching pattern \2%s\2:"), pattern);

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		struct mygroup *mg = group(mt);
		continue_if_fail(mt != NULL);
		continue_if_fail(mg != NULL);

		if (!match(pattern, entity(mg)->name))
		{
			command_success_nodata(si, "- %s (%s)", entity(mg)->name, mygroup_founder_names(mg));
			matches++;
		}
	}

	if (matches == 0)
		command_success_nodata(si, _("No groups matched pattern \2%s\2"), pattern);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 match for pattern \2%s\2"), N_("\2%u\2 matches for pattern \2%s\2"), matches), matches, pattern);

	logcommand(si, CMDLOG_ADMIN, "LIST: \2%s\2 (\2%u\2 matches)", pattern, matches);
}

static struct command gs_list = {
	.name           = "LIST",
	.desc           = N_("List registered groups."),
	.access         = PRIV_GROUP_AUSPEX,
	.maxparc        = 1,
	.cmd            = &gs_cmd_list,
	.help           = { .path = "groupserv/list" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_list);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_list);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/list", MODULE_UNLOAD_CAPABILITY_OK)
