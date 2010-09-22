/*
 * Copyright (c) 2005-2006 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for OS UPTIME
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/uptime", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_uptime(sourceinfo_t *si, int parc, char *parv[]);

command_t os_uptime = { "UPTIME", N_("Shows services uptime and the number of registered nicks and channels."), PRIV_SERVER_AUSPEX, 1, os_cmd_uptime, { .path = "oservice/uptime" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_uptime);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_uptime);
}

static void os_cmd_uptime(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_GET, "UPTIME");

        command_success_nodata(si, "%s [%s]", PACKAGE_STRING, revision);
        command_success_nodata(si, _("Services have been up for %s"), timediff(CURRTIME - me.start));
        command_success_nodata(si, _("Registered accounts: %d"), cnt.myuser);
	if (!nicksvs.no_nick_ownership)
        	command_success_nodata(si, _("Registered nicknames: %d"), cnt.mynick);
        command_success_nodata(si, _("Registered channels: %d"), cnt.mychan);
        command_success_nodata(si, _("Users currently online: %d"), cnt.user - me.me->users);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
