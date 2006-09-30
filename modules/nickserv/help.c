/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ HELP command.
 *
 * $Id: help.c 6585 2006-09-30 22:10:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6585 2006-09-30 22:10:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree, *ns_helptree;

static void ns_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_help = { "HELP", "Displays contextual help information.", AC_NONE, 1, ns_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_help, ns_cmdtree);
	help_addentry(ns_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&ns_help, ns_cmdtree);
	help_delentry(ns_helptree, "HELP");
}

/* HELP <command> [params] */
void ns_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, "***** \2%s Help\2 *****", nicksvs.nick);
		if (nicksvs.no_nick_ownership)
		{
			command_success_nodata(si, "\2%s\2 allows users to \2'register'\2 an account for use with", nicksvs.nick);
			command_success_nodata(si, "\2%s\2. If a registered account is not used by the owner for %d days,", chansvs.nick, (config_options.expire / 86400));
			command_success_nodata(si, "\2%s\2 will drop the account, allowing it to be reregistered.", nicksvs.nick);
		}
		else
		{
			command_success_nodata(si, "\2%s\2 allows users to \2'register'\2 a nickname, and stop", nicksvs.nick);
			command_success_nodata(si, "others from using that nick. \2%s\2 allows the owner of a", nicksvs.nick);
			command_success_nodata(si, "nickname to disconnect a user from the network that is using", nicksvs.nick);
			command_success_nodata(si, "their nickname. If a registered nick is not used by the owner for %d days,", (config_options.expire / 86400));
			command_success_nodata(si, "\2%s\2 will drop the nickname, allowing it to be reregistered.", nicksvs.nick);
		}
		command_success_nodata(si, " ");
		command_success_nodata(si, "For more information on a command, type:");
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", nicksvs.disp);
		command_success_nodata(si, " ");

		command_help_short(nicksvs.nick, si->su->nick, ns_cmdtree, "REGISTER IDENTIFY GHOST INFO LISTCHANS SET HOLD MARK FREEZE");

		command_success_nodata(si, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		command_success_nodata(si, "***** \2%s Help\2 *****", nicksvs.nick);
		command_help(nicksvs.nick, si->su->nick, ns_cmdtree);
		command_success_nodata(si, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(si, command, ns_helptree);
}
