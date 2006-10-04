/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODE command.
 *
 * $Id: mode.c 6657 2006-10-04 21:22:47Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/mode", FALSE, _modinit, _moddeinit,
	"$Id: mode.c 6657 2006-10-04 21:22:47Z jilles $",
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
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODE");
                command_fail(si, fault_needmoreparams, "Syntax: MODE <channel> <parameters>");
                return;
        }

	c = channel_find(channel);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, "Channel \2%s\2 does not exist.", channel);
                return;
	}

	wallops("\2%s\2 is using MODE on \2%s\2 (set: \2%s\2)",
		get_oper_name(si), channel, mode);
	snoop("MODE: \2%s\2 \2%s\2 by \2%s\2", channel, mode, get_oper_name(si));
	logcommand(si, CMDLOG_SET, "MODE %s %s", channel, mode);

	modeparc = sjtoken(mode, ' ', modeparv);

	channel_mode(si->service->me, c, modeparc, modeparv);
	command_success_nodata(si, "Set modes \2%s\2 on \2%s\2.", mode, channel);
}

