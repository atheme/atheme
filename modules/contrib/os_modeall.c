/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODEALL
 * command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/os_modeall", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modeall(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modeall = { "MODEALL", N_("Changes modes on all channels."), PRIV_OMODE, 2, os_cmd_modeall, { .path = "contrib/os_modeall" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_modeall);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_modeall);
}

static void set_channel_mode(service_t *s, channel_t *c, int modeparc, char *modeparv[])
{
	channel_mode(s->me, c, modeparc, modeparv);
}

static void os_cmd_modeall(sourceinfo_t *si, int parc, char *parv[])
{
	char *mode = parv[0];
	channel_t *c;
	int modeparc;
	char *modeparv[256];
	mowgli_patricia_iteration_state_t state;
	int count = 0;

        if (!mode)
        {
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODEALL");
                command_fail(si, fault_needmoreparams, _("Syntax: MODEALL <parameters>"));
                return;
        }

	modeparc = sjtoken(mode, ' ', modeparv);

	MOWGLI_PATRICIA_FOREACH(c, &state, chanlist)
	{
		set_channel_mode(si->service, c, modeparc, modeparv);
		count++;
	}

	command_success_nodata(si, _("Set modes \2%s\2 on \2%d\2 channels."), mode, count);
	wallops("\2%s\2 is using MODEALL (set: \2%s\2)",
		get_oper_name(si), mode);
	logcommand(si, CMDLOG_ADMIN, "MODEALL: \2%s\2", mode);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
