/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the MemoServ HELP command.
 *
 * $Id: help.c 4613 2006-01-19 23:52:30Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 4613 2006-01-19 23:52:30Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ms_cmdtree;
list_t *ms_helptree;

static void ms_cmd_help(char *origin);

command_t ms_help = { "HELP", "Displays contextual help information.", AC_NONE, ms_cmd_help };

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");

	command_add(&ms_help, ms_cmdtree);
	help_addentry(ms_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&ms_help, ms_cmdtree);
	help_delentry(ms_helptree, "HELP");
}

/* HELP <command> [params] */
void ms_cmd_help(char *origin)
{
	user_t *u = user_find_named(origin);
	char *command = strtok(NULL, "");

	if (!command)
	{
		notice(memosvs.nick, origin, "***** \2%s Help\2 *****", memosvs.nick);
		notice(memosvs.nick, origin, "\2%s\2 allows users to send memos to registered users.", memosvs.nick);
		notice(memosvs.nick, origin, " ");
		notice(memosvs.nick, origin, "For more information on a command, type:");
		notice(memosvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", memosvs.disp);
		notice(memosvs.nick, origin, " ");

		command_help(memosvs.nick, origin, ms_cmdtree);

		notice(memosvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(memosvs.nick, memosvs.disp, origin, command, ms_helptree);
}
