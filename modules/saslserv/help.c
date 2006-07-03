/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the SaslServ HELP command.
 *
 * $Id: help.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ss_cmdtree, *ss_helptree;

static void ss_cmd_help(char *origin);

command_t ss_help = { "HELP", "Displays contextual help information.", AC_NONE, ss_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ss_cmdtree, "saslserv/main", "ss_cmdtree");
	MODULE_USE_SYMBOL(ss_helptree, "saslserv/main", "ss_helptree");

	command_add(&ss_help, ss_cmdtree);
	help_addentry(ss_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&ss_help, ss_cmdtree);
	help_delentry(ss_helptree, "HELP");
}

/* HELP <command> [params] */
void ss_cmd_help(char *origin)
{
	char *command = strtok(NULL, "");

	if (!command)
	{
		notice(saslsvs.nick, origin, "***** \2%s Help\2 *****", saslsvs.nick);
		notice(saslsvs.nick, origin, "\2%s\2 allows users to modify settings concerning secure", saslsvs.nick);
		notice(saslsvs.nick, origin, "connection to the network. It may, for example, be used to");
		notice(saslsvs.nick, origin, "set keys or fingerprints.");
		notice(saslsvs.nick, origin, " ");
		notice(saslsvs.nick, origin, "For more information on a command, type:");
		notice(saslsvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", saslsvs.disp);
		notice(saslsvs.nick, origin, " ");

		command_help(saslsvs.nick, origin, ss_cmdtree);

		notice(saslsvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		notice(saslsvs.nick, origin, "***** \2%s Help\2 *****", saslsvs.nick);
		command_help(saslsvs.nick, origin, ss_cmdtree);
		notice(saslsvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(saslsvs.nick, saslsvs.disp, origin, command, ss_helptree);
}
