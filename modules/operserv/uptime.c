/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for OS UPTIME
 *
 * $Id: uptime.c 3601 2005-11-06 23:36:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/uptime", FALSE, _modinit, _moddeinit,
	"$Id: uptime.c 3601 2005-11-06 23:36:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_uptime(char *origin);

command_t os_uptime = { "UPTIME", "Shows services uptime and the number of registered nicks and channels.",
                        AC_IRCOP, os_cmd_uptime };

list_t *os_cmdtree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
        command_add(&os_uptime, os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_uptime, os_cmdtree);
}

static void os_cmd_uptime(char *origin)
{
        user_t *u = user_find(origin);

        snoop("UPTIME: \2%s\2", origin);
	logcommand(opersvs.me, user_find(origin), CMDLOG_GET, "UPTIME");

        notice(opersvs.nick, origin, "atheme-%s [%s] #%s", version, revision, generation);
        notice(opersvs.nick, origin, "Services have been up for %s", timediff(CURRTIME - me.start));
        notice(opersvs.nick, origin, "Registered nicknames: %d", cnt.myuser);
        notice(opersvs.nick, origin, "Registered channels: %d", cnt.mychan);
        notice(opersvs.nick, origin, "Users currently online: %d", cnt.user);

}

