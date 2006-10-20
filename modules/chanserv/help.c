/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 6701 2006-10-20 16:38:40Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 6701 2006-10-20 16:38:40Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_help(sourceinfo_t *si, int parc, char *parv[]);
static void fc_cmd_help(char *origin, char *chan);

command_t cs_help = { "HELP", "Displays contextual help information.",
                        AC_NONE, 1, cs_cmd_help };
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
	notice(chansvs.nick, origin, "More commands are supported via \2/%s%s <command>\2,", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
        notice(chansvs.nick, origin, "use \2/%s%s HELP\2 for more information.", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
        notice(chansvs.nick, origin, " ");

        notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
        return;
}

/* HELP <command> [params] */
static void cs_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	/* strip off channel name for fantasy commands */
	if (si->c)
	{
		command = strchr(command, ' ');
		if (command != NULL)
			command++;
	}

	if (!command)
	{
		command_success_nodata(si, "***** \2%s Help\2 *****", chansvs.nick);
		command_success_nodata(si, "\2%s\2 gives normal users the ability to maintain control", chansvs.nick);
		command_success_nodata(si, "of a channel, without the need of a bot. Channel takeovers are");
		command_success_nodata(si, "virtually impossible when a channel is registered with \2%s\2.", chansvs.nick);
		command_success_nodata(si, "Registration is a quick and painless process. Once registered,");
		command_success_nodata(si, "the founder can maintain complete and total control over the channel.");
		if (config_options.expire > 0)
		{
			command_success_nodata(si, "Please note that channels will expire after %d days of inactivity,", (config_options.expire / 86400));
			command_success_nodata(si, "or if there are no eligible channel successors.");
			command_success_nodata(si, "Activity is defined as a user with one of +vhHoOsrRf being on the channel.");
		}
		else
		{
			command_success_nodata(si, "Please note that channels will expire if there are no eligible channel successors.");
		}
		command_success_nodata(si, "Successors are primarily those who have the +R flag");
		command_success_nodata(si, "set on their account in the channel, although other");
		command_success_nodata(si, "people may be chosen depending on their access");
		command_success_nodata(si, "level and activity.");
		command_success_nodata(si, " ");
		command_success_nodata(si, "For more information on a command, type:");
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
		command_success_nodata(si, " ");
		command_success_nodata(si, "For a verbose listing of all commands, type:");
		command_success_nodata(si, "\2/%s%s help commands\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
		command_success_nodata(si, " ");

		command_help_short(si, cs_cmdtree, "REGISTER OP INVITE UNBAN FLAGS RECOVER SET");

		command_success_nodata(si, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("COMMANDS", command))
	{
		command_success_nodata(si, "***** \2%s Help\2 *****", chansvs.nick);
		command_help(si, cs_cmdtree);
		command_success_nodata(si, "***** \2End of Help\2 *****");
		return;
	}

	help_display(si, command, cs_helptree);
}
