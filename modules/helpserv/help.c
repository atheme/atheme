/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the helpserv HELP command.
 *
 * $Id: help.c 6317 2006-09-06 20:03:32Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6317 2006-09-06 20:03:32Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *hs_cmdtree;
list_t *hs_helptree;

static void hs_cmd_help(char *origin);

command_t hs_help = { "HELP", "Displays contextual help information.", AC_NONE, hs_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "helpserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "helpserv/main", "hs_helptree");

	command_add(&hs_help, hs_cmdtree);
	help_addentry(hs_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&hs_help, hs_cmdtree);
	help_delentry(hs_helptree, "HELP");
}

/* HELP <command> [params] */
void hs_cmd_help(char *origin)
{
	char *command = strtok(NULL, "");

	if (!command)
	{
		notice(helpsvs.nick, origin, "***** \2%s Help\2 *****", helpsvs.nick);
		notice(helpsvs.nick, origin, "\2%s\2 manages help requests for support staff.", helpsvs.nick);
		notice(helpsvs.nick, origin, " ");
		notice(helpsvs.nick, origin, "For more information on a command, type:");
		notice(helpsvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", helpsvs.disp);
		notice(helpsvs.nick, origin, " ");

		command_help(helpsvs.nick, origin, hs_cmdtree);

		notice(helpsvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	help_display(helpsvs.nick, helpsvs.disp, origin, command, hs_helptree);
}
