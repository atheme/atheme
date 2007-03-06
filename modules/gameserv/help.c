/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the MemoServ HELP command.
 *
 * $Id: help.c 6593 2006-10-01 18:51:45Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"gameserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6593 2006-10-01 18:51:45Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *gs_cmdtree;
list_t *gs_helptree;

static void gs_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_help = { "HELP", "Displays contextual help information.", AC_NONE, 2, gs_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(gs_cmdtree, "gameserv/main", "gs_cmdtree");
	MODULE_USE_SYMBOL(gs_helptree, "gameserv/main", "gs_helptree");

	command_add(&gs_help, gs_cmdtree);
	help_addentry(gs_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&gs_help, gs_cmdtree);
	help_delentry(gs_helptree, "HELP");
}

/* HELP <command> [params] */
void gs_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), gamesvs.nick);
		command_success_nodata(si, _("\2%s\2 provides games and tools for playing games to the network."), gamesvs.nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", gamesvs.disp);
		command_success_nodata(si, " ");

		command_help(si, gs_cmdtree);

		command_success_nodata(si, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(si, command, gs_helptree);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
