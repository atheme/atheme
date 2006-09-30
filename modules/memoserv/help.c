/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the MemoServ HELP command.
 *
 * $Id: help.c 6585 2006-09-30 22:10:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6585 2006-09-30 22:10:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ms_cmdtree;
list_t *ms_helptree;

static void ms_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_help = { "HELP", "Displays contextual help information.", AC_NONE, 2, ms_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

	command_add(&ms_help, ms_cmdtree);
	help_addentry(ms_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&ms_help, ms_cmdtree);
	help_delentry(ms_helptree, "HELP");
}

/* HELP <command> [params] */
void ms_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, "***** \2%s Help\2 *****", memosvs.nick);
		command_success_nodata(si, "\2%s\2 allows users to send memos to registered users.", memosvs.nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, "For more information on a command, type:");
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", memosvs.disp);
		command_success_nodata(si, " ");

		command_help(memosvs.nick, si->su->nick, ms_cmdtree);

		command_success_nodata(si, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(si, command, ms_helptree);
}
