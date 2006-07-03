/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: clear.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear", FALSE, _modinit, _moddeinit,
	"$Id: clear.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear(char *origin);

command_t cs_clear = { "CLEAR", "Channel removal toolkit.",
                        AC_NONE, cs_cmd_clear };

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

static void cs_cmd_clear(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *cmd = strtok(NULL, " ");

	if (!chan || !cmd)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLEAR");
		notice(chansvs.nick, origin, "Syntax: CLEAR <#channel> <command>");
		return;
	}

	fcommand_exec(chansvs.me, chan, origin, cmd, &cs_clear_cmds);
}
