/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 2553 2005-10-04 06:33:01Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 2553 2005-10-04 06:33:01Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ cs_help_commands[] = {
  { "SET FOUNDER",   AC_NONE, "help/cservice/set_founder"   },
  { "SET MLOCK",     AC_NONE, "help/cservice/set_mlock"     },
  { "SET SECURE",    AC_NONE, "help/cservice/set_secure"    },
  { "SET SUCCESSOR", AC_NONE, "help/cservice/set_successor" },
  { "SET VERBOSE",   AC_NONE, "help/cservice/set_verbose"   },
  { "SET URL",       AC_NONE, "help/cservice/set_url"       },
  { "SET EMAIL",     AC_NONE, "help/cservice/set_email"     },
  { "SET ENTRYMSG",  AC_NONE, "help/cservice/set_entrymsg"  },
  { "SET PROPERTY",  AC_NONE, "help/cservice/set_property"  },
  { "SET STAFFONLY", AC_IRCOP, "help/cservice/set_staffonly" },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static void cs_cmd_help(char *origin);
static void fc_cmd_help(char *origin, char *chan);


command_t cs_help = { "HELP", "Displays contextual help information.",
                        AC_NONE, cs_cmd_help };
fcommand_t fc_help = { "!help", AC_NONE, fc_cmd_help };


list_t *cs_cmdtree, *cs_fcmdtree, *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
        cs_fcmdtree = module_locate_symbol("chanserv/main", "cs_fcmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");
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
	cs_cmd_help(origin);
	return;
}

/* HELP <command> [params] */
static void cs_cmd_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(chansvs.nick, origin, "***** \2%s Help\2 *****", chansvs.nick);
		notice(chansvs.nick, origin, "\2%s\2 gives normal users the ability to maintain control", chansvs.nick);
		notice(chansvs.nick, origin, "of a channel, without the need of a bot. Channel takeovers are");
		notice(chansvs.nick, origin, "virtually impossible when a channel is registered with \2%s\2.", chansvs.nick);
		notice(chansvs.nick, origin, "Registration is a quick and painless process. Once registered,", chansvs.nick);
		notice(chansvs.nick, origin, "the founder can maintain complete and total control over the channel.");
		notice(chansvs.nick, origin, "Please note that channels will expire after %d days of inactivity,", (config_options.expire / 86400));
		notice(chansvs.nick, origin, "or if there are no eligible channel successors.");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "For more information on a command, type:");
		notice(chansvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);
		notice(chansvs.nick, origin, " ");

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
		notice(chansvs.nick, origin, "\2FOUNDER\2       Sets you founder of a channel.");
		notice(chansvs.nick, origin, "\2MLOCK\2         Sets channel mode lock.");
		notice(chansvs.nick, origin, "\2SECURE\2        Prevents unauthorized people from " "gaining operator status.");
		notice(chansvs.nick, origin, "\2SUCCESSOR\2     Sets a channel successor.");
		notice(chansvs.nick, origin, "\2VERBOSE\2       Notifies channel about access list " "modifications.");
		notice(chansvs.nick, origin, "\2URL\2           Sets the channel URL.");
		notice(chansvs.nick, origin, "\2EMAIL\2         Sets the channel e-mail address.");
		notice(chansvs.nick, origin, "\2ENTRYMSG\2      Sets the channel's entry message.");
		notice(chansvs.nick, origin, "\2PROPERTY\2      Manipulates channel metadata.");
		notice(chansvs.nick, origin, " ");

		if (is_sra(u->myuser) || is_ircop(u))
		{
			notice(chansvs.nick, origin, "The following IRCop commands are available.");
			notice(chansvs.nick, origin, "\2STAFFONLY\2     Sets the channel as staff-only. (Non staff-members are kickbanned.)");
			notice(chansvs.nick, origin, " ");
		}

		notice(chansvs.nick, origin, "For more specific help use \2HELP SET \37command\37\2.");

		notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(chansvs.nick, origin, command, cs_helptree)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(chansvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			notice(chansvs.nick, origin, "***** \2%s Help\2 *****", chansvs.nick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", chansvs.disp);

				if (buf[0])
					notice(chansvs.nick, origin, "%s", buf);
				else
					notice(chansvs.nick, origin, " ");
			}

			fclose(help_file);

			notice(chansvs.nick, origin, "***** \2End of Help\2 *****");
		}
		else if (c->func)
			c->func(origin);
		else
			notice(chansvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
