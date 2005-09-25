/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the NickServ HELP command.
 *
 * $Id: help.c 2361 2005-09-25 03:05:34Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 2361 2005-09-25 03:05:34Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *us_cmdtree;

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ us_help_commands[] = {
  { "REGISTER", AC_NONE,  "help/userserv/register" },
  { "VERIFY",   AC_NONE,  "help/userserv/verify"   },
  { "IDENTIFY", AC_NONE,  "help/userserv/identify" },
  { "LOGOUT",	AC_NONE,  "help/userserv/logout"   },
  { "HELP",     AC_NONE,  "help/help"              },
  { "INFO",     AC_NONE,  "help/userserv/info"     },
  { "DROP",     AC_NONE,  "help/userserv/drop"     },
  { "GHOST",    AC_NONE,  "help/userserv/ghost"    },
  { "STATUS",   AC_NONE,  "help/userserv/status"   },
  { "TAXONOMY", AC_NONE,  "help/userserv/taxonomy" },
  { "LINK",     AC_NONE,  "help/userserv/link"     },
  { "SENDPASS", AC_IRCOP, "help/userserv/sendpass" },
  { "LISTMAIL", AC_IRCOP, "help/userserv/listmail" },
  { "MARK",     AC_IRCOP, "help/userserv/mark"     },
  { "LIST",     AC_IRCOP, "help/userserv/list"     },
  { "HOLD",     AC_SRA,   "help/userserv/hold"     },
  { "MYACCESS", AC_NONE,  "help/userserv/myaccess" },
  { "SET EMAIL",     AC_NONE, "help/userserv/set_email"     },
  { "SET HIDEMAIL",  AC_NONE, "help/userserv/set_hidemail"  },
  { "SET NEVEROP",   AC_NONE, "help/userserv/set_neverop"   },
  { "SET NOOP",      AC_NONE, "help/userserv/set_noop"      },
  { "SET PASSWORD",  AC_NONE, "help/userserv/set_password"  },
  { "SET PROPERTY",  AC_NONE, "help/userserv/set_property"  },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static void us_cmd_help(char *origin);

command_t us_help = { "HELP", "Displays contextual help information.", AC_NONE, us_cmd_help };

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_help, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_help, us_cmdtree);
}

/* HELP <command> [params] */
void us_cmd_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(usersvs.nick, origin, "***** \2%s Help\2 *****", usersvs.nick);
		notice(usersvs.nick, origin, "\2%s\2 allows users to \2'register'\2 an account for use with", usersvs.nick);
		notice(usersvs.nick, origin, "\2%s\2. If a registered account is not used by the owner for %d days,", (config_options.expire / 86400), chansvs.nick);
		notice(usersvs.nick, origin, "\2%s\2 will drop the account, allowing it to be reregistered.", usersvs.nick);
		notice(usersvs.nick, origin, " ");
		notice(usersvs.nick, origin, "For more information on a command, type:");
		notice(usersvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", usersvs.disp);
		notice(usersvs.nick, origin, " ");

		command_help(usersvs.nick, origin, us_cmdtree);

		notice(usersvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	if (!strcasecmp("SET", command))
	{
		notice(usersvs.nick, origin, "***** \2%s Help\2 *****", usersvs.nick);
		notice(usersvs.nick, origin, "Help for \2SET\2:");
		notice(usersvs.nick, origin, " ");
		notice(usersvs.nick, origin, "SET allows you to set various control flags");
		notice(usersvs.nick, origin, "for accounts that change the way certain operations");
		notice(usersvs.nick, origin, "are performed on them.");
		notice(usersvs.nick, origin, " ");
		notice(usersvs.nick, origin, "The following commands are available.");
		notice(usersvs.nick, origin, "\2EMAIL\2         Changes the email address associated with a account.");
		notice(usersvs.nick, origin, "\2HIDEMAIL\2      Hides a account's email address");
		notice(usersvs.nick, origin, "\2NOOP\2       Prevents services from automatically setting modes associated with access lists.");
		notice(usersvs.nick, origin, "\2NEVEROP\2          Prevents you from being added to access lists.");
		notice(usersvs.nick, origin, "\2PASSWORD\2      Change the password of a account.");
		notice(usersvs.nick, origin, "\2PROPERTY\2      Manipulates metadata entries associated with a account.");
		notice(usersvs.nick, origin, " ");

#if 0		/* currently unused */
		if (is_sra(u->myuser))
		{
			notice(usersvs.nick, origin, "The following SRA commands are available.");
			notice(usersvs.nick, origin, "\2HOLD\2          Prevents services from expiring a account.");
			notice(usersvs.nick, origin, " ");
		}
#endif

		notice(usersvs.nick, origin, "For more specific help use \2HELP SET \37command\37\2.");

		notice(usersvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(usersvs.nick, origin, command, us_help_commands)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(usersvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			notice(usersvs.nick, origin, "***** \2%s Help\2 *****", usersvs.nick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", usersvs.disp);

				if (buf[0])
					notice(usersvs.nick, origin, "%s", buf);
				else
					notice(usersvs.nick, origin, " ");
			}

			fclose(help_file);

			notice(usersvs.nick, origin, "***** \2End of Help\2 *****");
		}
		else
			notice(usersvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
