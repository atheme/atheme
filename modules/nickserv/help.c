/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ HELP command.
 *
 * $Id: help.c 2133 2005-09-05 01:19:23Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 2133 2005-09-05 01:19:23Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ns_cmdtree;

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ ns_help_commands[] = {
  { "REGISTER", AC_NONE,  "help/nickserv/register" },
  { "VERIFY",   AC_NONE,  "help/nickserv/verify"   },
  { "IDENTIFY", AC_NONE,  "help/nickserv/identify" },
  { "LOGOUT",	AC_NONE,  "help/nickserv/logout"   },
  { "HELP",     AC_NONE,  "help/help"              },
  { "INFO",     AC_NONE,  "help/nickserv/info"     },
  { "DROP",     AC_NONE,  "help/nickserv/drop"     },
  { "GHOST",    AC_NONE,  "help/nickserv/ghost"    },
  { "STATUS",   AC_NONE,  "help/nickserv/status"   },
  { "TAXONOMY", AC_NONE,  "help/nickserv/taxonomy" },
  { "LINK",     AC_NONE,  "help/nickserv/link"     },
  { "SENDPASS", AC_IRCOP, "help/nickserv/sendpass" },
  { "LISTMAIL", AC_IRCOP, "help/nickserv/listmail" },
  { "MARK",     AC_IRCOP, "help/nickserv/mark"     },
  { "LIST",     AC_IRCOP, "help/nickserv/list"     },
  { "HOLD",     AC_SRA,   "help/nickserv/hold"     },
  { "MYACCESS", AC_NONE,  "help/nickserv/myaccess" },
  { "SET EMAIL",     AC_NONE, "help/nickserv/set_email"     },
  { "SET HIDEMAIL",  AC_NONE, "help/nickserv/set_hidemail"  },
  { "SET NEVEROP",   AC_NONE, "help/nickserv/set_neverop"   },
  { "SET NOOP",      AC_NONE, "help/nickserv/set_noop"      },
  { "SET PASSWORD",  AC_NONE, "help/nickserv/set_password"  },
  { "SET PROPERTY",  AC_NONE, "help/nickserv/set_property"  },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static void ns_cmd_help(char *origin);

command_t ns_help = { "HELP", "Displays contextual help information.", AC_NONE, ns_cmd_help };

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_help, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_help, ns_cmdtree);
}

/* HELP <command> [params] */
void ns_cmd_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

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

		command_help(nicksvs.nick, origin, ns_cmdtree);

		notice(nicksvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("SET", command))
	{
		notice(nicksvs.nick, origin, "***** \2%s Help\2 *****", nicksvs.nick);
		notice(nicksvs.nick, origin, "Help for \2SET\2:");
		notice(nicksvs.nick, origin, " ");
		notice(nicksvs.nick, origin, "SET allows you to set various control flags");
		notice(nicksvs.nick, origin, "for nicknames that change the way certain operations");
		notice(nicksvs.nick, origin, "are performed on them.");
		notice(nicksvs.nick, origin, " ");
		notice(nicksvs.nick, origin, "The following commands are available.");
		notice(nicksvs.nick, origin, "\2EMAIL\2         Changes the email address associated with a nickname.");
		notice(nicksvs.nick, origin, "\2HIDEMAIL\2      Hides a nickname's email address");
		notice(nicksvs.nick, origin, "\2NOOP\2       Prevents services from automatically setting modes associated with access lists.");
		notice(nicksvs.nick, origin, "\2NEVEROP\2          Prevents you from being added to access lists.");
		notice(nicksvs.nick, origin, "\2PASSWORD\2      Change the password of a nickname.");
		notice(nicksvs.nick, origin, "\2PROPERTY\2      Manipulates metadata entries associated with a nickname.");
		notice(nicksvs.nick, origin, " ");

#if 0		/* currently unused */
		if (is_sra(u->myuser))
		{
			notice(nicksvs.nick, origin, "The following SRA commands are available.");
			notice(nicksvs.nick, origin, "\2HOLD\2          Prevents services from expiring a nickname.");
			notice(nicksvs.nick, origin, " ");
		}
#endif

		notice(nicksvs.nick, origin, "For more specific help use \2HELP SET \37command\37\2.");

		notice(nicksvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(nicksvs.nick, origin, command, ns_help_commands)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(nicksvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			notice(nicksvs.nick, origin, "***** \2%s Help\2 *****", nicksvs.nick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", nicksvs.disp);

				if (buf[0])
					notice(nicksvs.nick, origin, "%s", buf);
				else
					notice(nicksvs.nick, origin, " ");
			}

			fclose(help_file);

			notice(nicksvs.nick, origin, "***** \2End of Help\2 *****");
		}
		else
			notice(nicksvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
