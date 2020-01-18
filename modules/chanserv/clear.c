/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains code for the CService KICK functions.
 */

#include <atheme.h>

// This is imported by modules/chanserv/clear_*.so
extern mowgli_patricia_t *cs_clear_cmds;
mowgli_patricia_t *cs_clear_cmds = NULL;

static void
cs_help_clear(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, chansvs.me, "CLEAR", subcmd, cs_clear_cmds);
		return;
	}

	(void) help_display_prefix(si, chansvs.me);
	(void) command_success_nodata(si, _("Help for \2CLEAR\2:"));
	(void) help_display_newline(si);
	(void) command_success_nodata(si, _("CLEAR allows you to clear various aspects of a channel."));
	(void) help_display_newline(si);
	(void) command_help(si, cs_clear_cmds);
	(void) help_display_moreinfo(si, chansvs.me, "CLEAR");
	(void) help_display_suffix(si);
}

static void
cs_cmd_clear(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEAR");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: CLEAR <#channel> <command> [parameters]"));
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
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "CLEAR");
		(void) command_fail(si, fault_badparams, _("Syntax: CLEAR <#channel> <command> [parameters]"));
		return;
	}

	parv[0] = subcmd;
	parv[1] = target;

	(void) subcommand_dispatch_simple(chansvs.me, si, parc, parv, cs_clear_cmds, "CLEAR");
}

static struct command cs_clear = {
	.name           = "CLEAR",
	.desc           = N_("Channel removal toolkit."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_clear,
	.help           = { .func = &cs_help_clear },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	if (! (cs_clear_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_named_bind_command("chanserv", &cs_clear);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cs_clear);

	(void) mowgli_patricia_destroy(cs_clear_cmds, &command_delete_trie_cb, cs_clear_cmds);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/clear", MODULE_UNLOAD_CAPABILITY_OK)
