/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t os_help = { "HELP", "Displays contextual help information.", AC_NONE, 1, os_cmd_help };

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
static void os_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!has_any_privs(si->su))
	{
		notice(opersvs.nick, si->su->nick, "You are not authorized to use %s.", opersvs.nick);
		return;
	}

	if (!command)
	{
		notice(opersvs.nick, si->su->nick, "***** \2%s Help\2 *****", opersvs.nick);
		notice(opersvs.nick, si->su->nick, "\2%s\2 provides essential network management services, such as", opersvs.nick);
		notice(opersvs.nick, si->su->nick, "routing manipulation and access restriction. Please do not abuse");
		notice(opersvs.nick, si->su->nick, "your access to \2%s\2!", opersvs.nick);
		notice(opersvs.nick, si->su->nick, " ");
		notice(opersvs.nick, si->su->nick, "For information on a command, type:");
		notice(opersvs.nick, si->su->nick, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.disp);
		notice(opersvs.nick, si->su->nick, " ");

		command_help(opersvs.nick, si->su->nick, os_cmdtree);

		notice(opersvs.nick, si->su->nick, "***** \2End of Help\2 *****", opersvs.nick);
		return;
	}

	/* take the command through the hash table */
	help_display(opersvs.nick, opersvs.disp, si->su->nick, command, os_helptree);
}
