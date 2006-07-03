/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_help(char *origin);

command_t os_help = { "HELP", "Displays contextual help information.",
			AC_NONE, os_cmd_help };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_help, os_cmdtree);
	help_addentry(os_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&os_help, os_cmdtree);
	help_delentry(os_helptree, "HELP");
}

/* HELP <command> [params] */
static void os_cmd_help(char *origin)
{
	char *command = strtok(NULL, "");

	if (!has_any_privs(user_find_named(origin)))
	{
		notice(opersvs.nick, origin, "You are not authorized to use %s.", opersvs.nick);
		return;
	}

	if (!command)
	{
		notice(opersvs.nick, origin, "***** \2%s Help\2 *****", opersvs.nick);
		notice(opersvs.nick, origin, "\2%s\2 provides essential network management services, such as", opersvs.nick);
		notice(opersvs.nick, origin, "routing manipulation and access restriction. Please do not abuse");
		notice(opersvs.nick, origin, "your access to \2%s\2!", opersvs.nick);
		notice(opersvs.nick, origin, " ");
		notice(opersvs.nick, origin, "For information on a command, type:");
		notice(opersvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.disp);
		notice(opersvs.nick, origin, " ");

		command_help(opersvs.nick, origin, os_cmdtree);

		notice(opersvs.nick, origin, "***** \2End of Help\2 *****", opersvs.nick);
		return;
	}

	/* take the command through the hash table */
	help_display(opersvs.nick, opersvs.disp, origin, command, os_helptree);
}
