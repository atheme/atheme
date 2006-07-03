/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODE command.
 *
 * $Id: mode.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/mode", FALSE, _modinit, _moddeinit,
	"$Id: mode.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_mode(char *origin);

command_t os_mode = { "MODE", "Changes modes on channels.",
                        PRIV_OMODE, os_cmd_mode };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_mode, os_cmdtree);
	help_addentry(os_helptree, "MODE", "help/oservice/mode", NULL);
}

void _moddeinit()
{
	command_delete(&os_mode, os_cmdtree);
	help_delentry(os_helptree, "MODE");
}

static void os_cmd_mode(char *origin)
{
        char *channel = strtok(NULL, " ");
	char *mode = strtok(NULL, "");
	channel_t *c;
	int8_t parc;
	char *parv[256];

        if (!channel || !mode)
        {
                notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "MODE");
                notice(opersvs.nick, origin, "Syntax: MODE <parameters>");
                return;
        }

	c = channel_find(channel);
	if (!c)
	{
                notice(opersvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
                return;
	}

	wallops("\2%s\2 is using MODE on \2%s\2 (set: \2%s\2)",
		origin, channel, mode);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_SET, "MODE %s %s", channel, mode);

	parc = sjtoken(mode, ' ', parv);

	channel_mode(opersvs.me->me, c, parc, parv);
}

