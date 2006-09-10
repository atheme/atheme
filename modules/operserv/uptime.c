/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for OS UPTIME
 *
 * $Id: uptime.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/uptime", FALSE, _modinit, _moddeinit,
	"$Id: uptime.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_uptime(sourceinfo_t *si, int parc, char *parv[]);

command_t os_uptime = { "UPTIME", "Shows services uptime and the number of registered nicks and channels.", PRIV_SERVER_AUSPEX, 1, os_cmd_uptime };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_uptime, os_cmdtree);
	help_addentry(os_helptree, "UPTIME", "help/oservice/uptime", NULL);
}

void _moddeinit()
{
	command_delete(&os_uptime, os_cmdtree);
	help_delentry(os_helptree, "UPTIME");
}

static void os_cmd_uptime(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(opersvs.me, si->su, CMDLOG_GET, "UPTIME");

        notice(opersvs.nick, si->su->nick, "atheme-%s [%s] #%s", version, revision, generation);
        notice(opersvs.nick, si->su->nick, "Services have been up for %s", timediff(CURRTIME - me.start));
        notice(opersvs.nick, si->su->nick, "Registered nicknames: %d", cnt.myuser);
        notice(opersvs.nick, si->su->nick, "Registered channels: %d", cnt.mychan);
        notice(opersvs.nick, si->su->nick, "Users currently online: %d", cnt.user - me.me->users);
}

