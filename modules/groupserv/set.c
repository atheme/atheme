/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include <atheme.h>

#define IN_GROUPSERV_SET
#include "groupserv.h"

// Imported by other modules/groupserv/set_*.so
mowgli_patricia_t *gs_set_cmdtree = NULL;

static void
gs_help_set(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, si->service, "SET", subcmd, gs_set_cmdtree);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("Help for \2SET\2:"));
	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("SET allows you to set various control flags for\n"
	                                    "groups that change the way certain operations\n"
	                                    "are performed on them."));

	(void) help_display_newline(si);
	(void) command_help(si, gs_set_cmdtree);
	(void) help_display_moreinfo(si, si->service, "SET");
	(void) help_display_suffix(si);
}

static void
gs_cmd_set(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET <!group> <setting> [parameters]"));
		return;
	}

	char *subcmd;
	char *target;

	if (parv[0][0] == '!')
	{
		subcmd = parv[1];
		target = parv[0];
	}
	else if (parv[1][0] == '!')
	{
		subcmd = parv[0];
		target = parv[1];
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		(void) command_fail(si, fault_badparams, _("Syntax: SET <!group> <setting> [parameters]"));
		return;
	}

	parv[0] = subcmd;
	parv[1] = target;

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, gs_set_cmdtree, "SET");
}

static struct command gs_set = {
	.name           = "SET",
	.desc           = N_("Sets various control flags."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 3,
	.cmd            = &gs_cmd_set,
	.help           = { .func = &gs_help_set },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	if (! (gs_set_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_named_bind_command("groupserv", &gs_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("groupserv", &gs_set);

	(void) mowgli_patricia_destroy(gs_set_cmdtree, &command_delete_trie_cb, gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set", MODULE_UNLOAD_CAPABILITY_OK)
