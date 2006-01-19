/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for OS UPTIME
 *
 * $Id: uptime.c 4613 2006-01-19 23:52:30Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/uptime", FALSE, _modinit, _moddeinit,
	"$Id: uptime.c 4613 2006-01-19 23:52:30Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_uptime(char *origin);

command_t os_uptime = { "UPTIME", "Shows services uptime and the number of registered nicks and channels.",
                        PRIV_SERVER_AUSPEX, os_cmd_uptime };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");
        command_add(&os_uptime, os_cmdtree);
	help_addentry(os_helptree, "UPTIME", "help/oservice/uptime", NULL);
}

void _moddeinit()
{
	command_delete(&os_uptime, os_cmdtree);
	help_delentry(os_helptree, "UPTIME");
}

static void os_cmd_uptime(char *origin)
{
        user_t *u = user_find_named(origin);

	logcommand(opersvs.me, user_find_named(origin), CMDLOG_GET, "UPTIME");

        notice(opersvs.nick, origin, "atheme-%s [%s] #%s", version, revision, generation);
        notice(opersvs.nick, origin, "Services have been up for %s", timediff(CURRTIME - me.start));
        notice(opersvs.nick, origin, "Registered nicknames: %d", cnt.myuser);
        notice(opersvs.nick, origin, "Registered channels: %d", cnt.mychan);
        notice(opersvs.nick, origin, "Users currently online: %d", cnt.user - me.me->users);

}

