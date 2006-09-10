/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: clear.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear", FALSE, _modinit, _moddeinit,
	"$Id: clear.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_clear = { "CLEAR", "Channel removal toolkit.",
                        AC_NONE, 3, cs_cmd_clear };

list_t *cs_cmdtree;
list_t *cs_helptree;
list_t cs_clear_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_clear, cs_cmdtree);
	help_addentry(cs_helptree, "CLEAR", "help/cservice/clear", NULL);
}

void _moddeinit()
{
	command_delete(&cs_clear, cs_cmdtree);

	help_delentry(cs_helptree,  "CLEAR");
}

static void cs_cmd_clear(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;

	if (parc < 2)
	{
		notice(chansvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "CLEAR");
		notice(chansvs.nick, si->su->nick, "Syntax: CLEAR <#channel> <command> [parameters]");
		return;
	}
	
	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		notice(chansvs.nick, si->su->nick, STR_INVALID_PARAMS, "CLEAR");
		notice(chansvs.nick, si->su->nick, "Syntax: CLEAR <#channel> <command> [parameters]");
		return;
	}

	c = command_find(&cs_clear_cmds, cmd);
	if (c == NULL)
	{
		notice(chansvs.nick, si->su->nick, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.me->disp);
		return;
	}

	parv[1] = chan;
	command_exec(chansvs.me, si, c, parc - 1, parv + 1);
}
