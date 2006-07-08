/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 5784 2006-07-08 22:30:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 5784 2006-07-08 22:30:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_help(char *origin);
static void fc_cmd_help(char *origin, char *chan);

command_t cs_help = { "HELP", "Displays contextual help information.",
                        AC_NONE, cs_cmd_help };
fcommand_t fc_help = { "!help", AC_NONE, fc_cmd_help };

list_t *cs_cmdtree, *cs_fcmdtree, *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
        MODULE_USE_SYMBOL(cs_fcmdtree, "chanserv/main", "cs_fcmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_help, cs_cmdtree);
	fcommand_add(&fc_help, cs_fcmdtree);
	help_addentry(cs_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&cs_help, cs_cmdtree);
        fcommand_delete(&fc_help, cs_fcmdtree);
	help_delentry(cs_helptree, "HELP");
}

static void fc_cmd_help(char *origin, char *chan)
{
        node_t *n;
	char buf[BUFSIZE];
	char delim[2] = " ";
	int i=0;

        notice(chansvs.nick, origin, "***** \2%s Help\2 *****", chansvs.nick);
        notice(chansvs.nick, origin, "%s provides the following in-channel commands:", chansvs.nick);
        notice(chansvs.nick, origin, " ");
	
	buf[0]='\0';
	
	LIST_FOREACH(n, cs_fcmdtree->head)
        {
                fcommand_t *c = n->data;
         	i++;
		strlcat(buf, c->name, BUFSIZE);
		strlcat(buf, delim, BUFSIZE);

		if (c->access != NULL)
			continue;

		if (i==6)
		{
	        	notice(chansvs.nick, origin, " %s", buf);
			buf[0]='\0';
			i=0;
		}
        }

	if (i!=6) notice(chansvs.nick, origin, " %s", buf);

        notice(chansvs.nick, origin, " ");
	notice(chansvs.nick, origin, "In addition, the following commands are supported via");
        notice(chansvs.nick, origin, "\2/%s%s <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
        notice(chansvs.nick, origin, " ");

        command_help(chansvs.nick, origin, cs_cmdtree);

        notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
        return;
}

/* HELP <command> [params] */
static void cs_cmd_help(char *origin)
{
	user_t *u = user_find_named(origin);
	char *command = strtok(NULL, "");

	if (!command)
	{
		notice(chansvs.nick, origin, "***** \2%s Help\2 *****", chansvs.nick);
		notice(chansvs.nick, origin, "\2%s\2 gives normal users the ability to maintain control", chansvs.nick);
		notice(chansvs.nick, origin, "of a channel, without the need of a bot. Channel takeovers are");
		notice(chansvs.nick, origin, "virtually impossible when a channel is registered with \2%s\2.", chansvs.nick);
		notice(chansvs.nick, origin, "Registration is a quick and painless process. Once registered,");
		notice(chansvs.nick, origin, "the founder can maintain complete and total control over the channel.");
		if (config_options.expire > 0)
		{
			notice(chansvs.nick, origin, "Please note that channels will expire after %d days of inactivity,", (config_options.expire / 86400));
			notice(chansvs.nick, origin, "or if there are no eligible channel successors.");
			notice(chansvs.nick, origin, "Activity is defined as a user with one of +vhHoOsrRf being on the channel.");
		}
		else
		{
			notice(chansvs.nick, origin, "Please note that channels will expire if there are no eligible channel successors.");
		}
		notice(chansvs.nick, origin, "Successors are primarily those who have the +R flag");
		notice(chansvs.nick, origin, "set on their account in the channel, although other");
		notice(chansvs.nick, origin, "people may be chosen depending on their access");
		notice(chansvs.nick, origin, "level and activity.");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "For more information on a command, type:");
		notice(chansvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "For a verbose listing of all commands, type:");
		notice(chansvs.nick, origin, "\2/%s%s help commands\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
		notice(chansvs.nick, origin, " ");

		command_help_short(chansvs.nick, origin, cs_cmdtree, "REGISTER OP INVITE UNBAN FLAGS RECOVER SET");

		notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		notice(chansvs.nick, origin, "***** \2%s Help\2 *****", chansvs.nick);
		command_help(chansvs.nick, origin, cs_cmdtree);
		notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("SET", command))
	{
		notice(chansvs.nick, origin, "***** \2%s Help\2 *****", chansvs.nick);
		notice(chansvs.nick, origin, "Help for \2SET\2:");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "SET allows you to set various control flags");
		notice(chansvs.nick, origin, "for channels that change the way certain");
		notice(chansvs.nick, origin, "operations are performed on them.");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "The following commands are available.");
		notice(chansvs.nick, origin, "\2FOUNDER\2       Transfers foundership of a channel.");
		notice(chansvs.nick, origin, "\2MLOCK\2         Sets channel mode lock.");
		notice(chansvs.nick, origin, "\2SECURE\2        Prevents unauthorized people from " "gaining operator status.");
		notice(chansvs.nick, origin, "\2VERBOSE\2       Notifies channel about access list " "modifications.");
		notice(chansvs.nick, origin, "\2FANTASY\2       Allows or disallows in-channel commands.");
		notice(chansvs.nick, origin, "\2URL\2           Sets the channel URL.");
		notice(chansvs.nick, origin, "\2EMAIL\2         Sets the channel e-mail address.");
		notice(chansvs.nick, origin, "\2ENTRYMSG\2      Sets the channel's entry message.");
		notice(chansvs.nick, origin, "\2KEEPTOPIC\2     Enables topic retention.");
		notice(chansvs.nick, origin, "\2PROPERTY\2      Manipulates channel metadata.");
		notice(chansvs.nick, origin, " ");

		if (has_any_privs(u))
		{
			notice(chansvs.nick, origin, "The following IRCop commands are available.");
			notice(chansvs.nick, origin, "\2STAFFONLY\2     Sets the channel as staff-only. (Non staff-members are kickbanned.)");
			notice(chansvs.nick, origin, " ");
		}

		notice(chansvs.nick, origin, "For more specific help use \2HELP SET \37command\37\2.");

		notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	help_display(chansvs.nick, chansvs.disp, origin, command, cs_helptree);
}
