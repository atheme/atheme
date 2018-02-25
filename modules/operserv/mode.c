/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODE command.
 */

#include "atheme.h"

static void os_cmd_mode(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_mode = { "MODE", N_("Changes modes on channels."), PRIV_OMODE, 2, os_cmd_mode, { .path = "oservice/mode" } };

static void
mod_init(struct module *const restrict m)
{
        service_named_bind_command("operserv", &os_mode);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_mode);
}

static void
os_cmd_mode(struct sourceinfo *si, int parc, char *parv[])
{
        char *channel = parv[0];
	char *mode = parv[1];
	struct channel *c;
	int modeparc;
	char *modeparv[256];

        if (!channel || !mode)
        {
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODE");
                command_fail(si, fault_needmoreparams, _("Syntax: MODE <channel> <parameters>"));
                return;
        }

	c = channel_find(channel);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not exist."), channel);
                return;
	}

	wallops("\2%s\2 is using MODE on \2%s\2 (set: \2%s\2)",
		get_oper_name(si), channel, mode);
	logcommand(si, CMDLOG_ADMIN, "MODE: \2%s\2 on \2%s\2", mode, channel);
	command_success_nodata(si, _("Setting modes \2%s\2 on \2%s\2."), mode, channel);

	modeparc = sjtoken(mode, ' ', modeparv);

	channel_mode(si->service->me, c, modeparc, modeparv);
}

SIMPLE_DECLARE_MODULE_V1("operserv/mode", MODULE_UNLOAD_CAPABILITY_OK)
