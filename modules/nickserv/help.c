/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ HELP command.
 *
 * $Id: help.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree, *ns_helptree;

static void ns_cmd_help(char *origin);

command_t ns_help = { "HELP", "Displays contextual help information.", AC_NONE, ns_cmd_help };

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
void ns_cmd_help(char *origin)
{
	char *command = strtok(NULL, "");

	if (!command)
	{
		notice(nicksvs.nick, origin, "***** \2%s Help\2 *****", nicksvs.nick);
		notice(nicksvs.nick, origin, "\2%s\2 allows users to \2'register'\2 a nickname, and stop", nicksvs.nick);
		notice(nicksvs.nick, origin, "others from using that nick. \2%s\2 allows the owner of a", nicksvs.nick);
		notice(nicksvs.nick, origin, "nickname to disconnect a user from the network that is using", nicksvs.nick);
		notice(nicksvs.nick, origin, "their nickname. If a registered nick is not used by the owner for %d days,", (config_options.expire / 86400));
		notice(nicksvs.nick, origin, "\2%s\2 will drop the nickname, allowing it to be reregistered.", nicksvs.nick);
		notice(nicksvs.nick, origin, " ");
		notice(nicksvs.nick, origin, "For more information on a command, type:");
		notice(nicksvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", nicksvs.disp);
		notice(nicksvs.nick, origin, " ");

		command_help_short(nicksvs.nick, origin, ns_cmdtree, "REGISTER IDENTIFY GHOST INFO LISTCHANS SET HOLD MARK FREEZE");

		notice(nicksvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		notice(nicksvs.nick, origin, "***** \2%s Help\2 *****", nicksvs.nick);
		command_help(nicksvs.nick, origin, ns_cmdtree);
		notice(nicksvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(nicksvs.nick, nicksvs.disp, origin, command, ns_helptree);
}
