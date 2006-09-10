/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODE command.
 *
 * $Id: mode.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/mode", FALSE, _modinit, _moddeinit,
	"$Id: mode.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_mode(sourceinfo_t *si, int parc, char *parv[]);

command_t os_mode = { "MODE", "Changes modes on channels.", PRIV_OMODE, 2, os_cmd_mode };

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

static void os_cmd_mode(sourceinfo_t *si, int parc, char *parv[])
{
        char *channel = parv[0];
	char *mode = parv[1];
	channel_t *c;
	int modeparc;
	char *modeparv[256];

        if (!channel || !mode)
        {
                notice(opersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "MODE");
                notice(opersvs.nick, si->su->nick, "Syntax: MODE <parameters>");
                return;
        }

	c = channel_find(channel);
	if (!c)
	{
                notice(opersvs.nick, si->su->nick, "Channel \2%s\2 does not exist.", channel);
                return;
	}

	wallops("\2%s\2 is using MODE on \2%s\2 (set: \2%s\2)",
		si->su->nick, channel, mode);
	snoop("MODE: \2%s\2 \2%s\2 by \2%s\2", channel, mode, si->su->nick);
	logcommand(opersvs.me, si->su, CMDLOG_SET, "MODE %s %s", channel, mode);

	modeparc = sjtoken(mode, ' ', modeparv);

	channel_mode(opersvs.me->me, c, modeparc, modeparv);
}

