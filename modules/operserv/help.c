/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 6463 2006-09-25 13:03:41Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6463 2006-09-25 13:03:41Z jilles $",
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
		command_fail(si, fault_noprivs, "You are not authorized to use %s.", opersvs.nick);
		return;
	}

	if (!command)
	{
		command_success_nodata(si, "***** \2%s Help\2 *****", opersvs.nick);
		command_success_nodata(si, "\2%s\2 provides essential network management services, such as", opersvs.nick);
		command_success_nodata(si, "routing manipulation and access restriction. Please do not abuse");
		command_success_nodata(si, "your access to \2%s\2!", opersvs.nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, "For information on a command, type:");
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.disp);
		command_success_nodata(si, " ");

		command_help(opersvs.nick, si->su->nick, os_cmdtree);

		command_success_nodata(si, "***** \2End of Help\2 *****", opersvs.nick);
		return;
	}

	/* take the command through the hash table */
	help_display(opersvs.nick, opersvs.disp, si->su->nick, command, os_helptree);
}
