/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 */

#include "atheme.h"

// This is imported by modules/chanserv/clear_*.so
mowgli_patricia_t *cs_clear_cmds;

static void
cs_help_clear(struct sourceinfo *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), chansvs.me->disp);
		command_success_nodata(si, _("Help for \2CLEAR\2:"));
		command_success_nodata(si, " ");
		command_success_nodata(si, _("CLEAR allows you to clear various aspects of a channel."));
		command_success_nodata(si, " ");
		command_help(si, cs_clear_cmds);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information, use \2/msg %s HELP CLEAR \37command\37\2."), chansvs.me->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, cs_clear_cmds);
}

static void
cs_cmd_clear(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	struct command *c;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEAR");
		command_fail(si, fault_needmoreparams, _("Syntax: CLEAR <#channel> <command> [parameters]"));
		return;
	}

	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "CLEAR");
		command_fail(si, fault_badparams, _("Syntax: CLEAR <#channel> <command> [parameters]"));
		return;
	}

	c = command_find(cs_clear_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
		return;
	}

	parv[1] = chan;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static struct command cs_clear = { "CLEAR", N_("Channel removal toolkit."), AC_NONE, 3, cs_cmd_clear, { .func = cs_help_clear } };

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
        service_named_bind_command("chanserv", &cs_clear);

	cs_clear_cmds = mowgli_patricia_create(strcasecanon);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_clear);

	mowgli_patricia_destroy(cs_clear_cmds, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/clear", MODULE_UNLOAD_CAPABILITY_OK)
