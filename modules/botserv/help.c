/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the BotServ HELP command.
 *
 * $Id: help.c 7895 2009-01-24 02:40:03Z celestin $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"botserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 7895 2009-01-24 02:40:03Z celestin $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *bs_cmdtree;
list_t *bs_helptree;

static void bs_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t bs_help = { "HELP", N_(N_("Displays contextual help information.")), AC_NONE, 2, bs_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(bs_cmdtree, "botserv/main", "bs_cmdtree");
	MODULE_USE_SYMBOL(bs_helptree, "botserv/main", "bs_helptree");

	command_add(&bs_help, bs_cmdtree);
	help_addentry(bs_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&bs_help, bs_cmdtree);
	help_delentry(bs_helptree, "HELP");
}

/* HELP <command> [params] */
void bs_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), botsvs.nick);
		command_success_nodata(si, _("\2%s\2 allows users to request a custom bot for their channel."), botsvs.nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", botsvs.disp);
		command_success_nodata(si, " ");

		command_help(si, bs_cmdtree);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, command, bs_helptree);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
