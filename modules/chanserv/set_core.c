/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the ChanServ SET command.
 */

#include <atheme.h>

// This is imported by other modules/chanserv/set_*.so
extern mowgli_patricia_t *cs_set_cmdtree;
mowgli_patricia_t *cs_set_cmdtree = NULL;

static void
cs_help_set(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, chansvs.me, "SET", subcmd, cs_set_cmdtree);
		return;
	}

	(void) help_display_prefix(si, chansvs.me);
	(void) command_success_nodata(si, _("Help for \2SET\2:"));
	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("SET allows you to set various control flags for\n"
	                                    "channels that change the way certain operations\n"
	                                    "are performed on them."));

	(void) help_display_newline(si);
	(void) command_help(si, cs_set_cmdtree);
	(void) help_display_moreinfo(si, chansvs.me, "SET");
	(void) help_display_suffix(si);
}

static void
cs_cmd_set(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> <setting> [parameters]"));
		return;
	}

	char *subcmd;
	char *target;

	if (parv[0][0] == '#')
	{
		subcmd = parv[1];
		target = parv[0];
	}
	else if (parv[1][0] == '#')
	{
		subcmd = parv[0];
		target = parv[1];
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		(void) command_fail(si, fault_badparams, _("Syntax: SET <#channel> <setting> [parameters]"));
		return;
	}

	parv[0] = subcmd;
	parv[1] = target;

	(void) subcommand_dispatch_simple(chansvs.me, si, parc, parv, cs_set_cmdtree, "SET");
}

static struct command cs_set = {
	.name           = "SET",
	.desc           = N_("Sets various control flags."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_set,
	.help           = { .func = &cs_help_set },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	if (! (cs_set_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_named_bind_command("chanserv", &cs_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cs_set);

	(void) mowgli_patricia_destroy(cs_set_cmdtree, &command_delete_trie_cb, cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
