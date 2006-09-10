/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ HELP command.
 *
 * $Id: help.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *us_cmdtree;
list_t *us_helptree;

static void us_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t us_help = { "HELP", "Displays contextual help information.", AC_NONE, 1, us_cmd_help };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_help, us_cmdtree);
	help_addentry(us_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&us_help, us_cmdtree);
	help_delentry(us_helptree, "HELP");
}

/* HELP <command> [params] */
static void us_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		notice(usersvs.nick, si->su->nick, "***** \2%s Help\2 *****", usersvs.nick);
		notice(usersvs.nick, si->su->nick, "\2%s\2 allows users to \2'register'\2 an account for use with", usersvs.nick);
		notice(usersvs.nick, si->su->nick, "\2%s\2. If a registered account is not used by the owner for %d days,", chansvs.nick, (config_options.expire / 86400));
		notice(usersvs.nick, si->su->nick, "\2%s\2 will drop the account, allowing it to be reregistered.", usersvs.nick);
		notice(usersvs.nick, si->su->nick, " ");
		notice(usersvs.nick, si->su->nick, "For more information on a command, type:");
		notice(usersvs.nick, si->su->nick, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", usersvs.disp);
		notice(usersvs.nick, si->su->nick, " ");

		command_help_short(usersvs.nick, si->su->nick, us_cmdtree, "REGISTER LOGIN INFO LISTCHANS SET HOLD MARK FREEZE");

		notice(usersvs.nick, si->su->nick, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		notice(usersvs.nick, si->su->nick, "***** \2%s Help\2 *****", usersvs.nick);
		command_help(usersvs.nick, si->su->nick, us_cmdtree);
		notice(usersvs.nick, si->su->nick, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("SET", command))
	{
		notice(usersvs.nick, si->su->nick, "***** \2%s Help\2 *****", usersvs.nick);
		notice(usersvs.nick, si->su->nick, "Help for \2SET\2:");
		notice(usersvs.nick, si->su->nick, " ");
		notice(usersvs.nick, si->su->nick, "SET allows you to set various control flags");
		notice(usersvs.nick, si->su->nick, "for accounts that change the way certain operations");
		notice(usersvs.nick, si->su->nick, "are performed on them.");
		notice(usersvs.nick, si->su->nick, " ");
		notice(usersvs.nick, si->su->nick, "The following commands are available.");
		notice(usersvs.nick, si->su->nick, "\2EMAIL\2         Changes the email address associated with a account.");
		notice(usersvs.nick, si->su->nick, "\2HIDEMAIL\2      Hides a account's email address");
		notice(usersvs.nick, si->su->nick, "\2NOOP\2          Prevents services from automatically setting modes associated with access lists.");
		notice(usersvs.nick, si->su->nick, "\2NEVEROP\2       Prevents you from being added to access lists.");
		notice(usersvs.nick, si->su->nick, "\2PASSWORD\2      Change the password of a account.");
		notice(usersvs.nick, si->su->nick, "\2PROPERTY\2      Manipulates metadata entries associated with a account.");
		notice(usersvs.nick, si->su->nick, " ");

#if 0		/* currently unused */
		if (is_soper(u->myuser))
		{
			notice(usersvs.nick, si->su->nick, "The following SRA commands are available.");
			notice(usersvs.nick, si->su->nick, "\2HOLD\2          Prevents services from expiring a account.");
			notice(usersvs.nick, si->su->nick, " ");
		}
#endif

		notice(usersvs.nick, si->su->nick, "For more specific help use \2HELP SET \37command\37\2.");

		notice(usersvs.nick, si->su->nick, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	help_display(usersvs.nick, usersvs.disp, si->su->nick, command, us_helptree);
}
