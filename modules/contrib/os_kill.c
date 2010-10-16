/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Kill users through services, requested by christel@freenode.
 *
 * This differs from the ircd /kill command in that it does not show to
 * normal users who issued the kill, although the reason will usually be
 * shown. This is useful in cases where a kline would normally be used,
 * but would not remove the user, but the user cannot (fully) reconnect.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/kill", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_kill(sourceinfo_t *si, int parc, char *parv[]);

command_t os_kill = { "KILL", "Kill a user with Services.", PRIV_OMODE, 2, os_cmd_kill, { .path = "contrib/kill" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_kill);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_kill);
}

static void os_cmd_kill(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *target;

	if(!parv[0] || !parv[1])
	{
		command_fail(si, fault_badparams, "Usage: \2KILL\2 <target> <reason>");
		return;
	}

	if(!(target = user_find_named(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on the network", parv[0]);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "KILL: \2%s\2 (reason: \2%s\2)", target->nick, parv[1]);
	command_success_nodata(si, "\2%s\2 has been killed.", target->nick);

	kill_user(si->service->me, target, "Requested: %s", parv[1]);
}
