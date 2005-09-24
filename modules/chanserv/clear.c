/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: clear.c 2345 2005-09-24 02:44:06Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear", FALSE, _modinit, _moddeinit,
	"$Id: clear.c 2345 2005-09-24 02:44:06Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear(char *origin);

command_t cs_clear = { "CLEAR", "Channel removal toolkit.",
                        AC_NONE, cs_cmd_clear };

list_t *cs_cmdtree;
list_t cs_clear_cmds;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");

        command_add(&cs_clear, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_clear, cs_cmdtree);
}

static void cs_cmd_clear(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *cmd = strtok(NULL, " ");

	if (!chan || !cmd)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2CLEAR\2.");
		notice(chansvs.nick, origin, "Syntax: CLEAR <#channel> <command>");
		return;
	}

	fcommand_exec(chan, origin, cmd, &cs_clear_cmds);
}
