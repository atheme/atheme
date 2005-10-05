/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the MemoServ HELP command.
 *
 * $Id: help.c 2597 2005-10-05 06:37:06Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 2597 2005-10-05 06:37:06Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *ms_cmdtree;

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ ms_help_commands[] = {
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static void ms_cmd_help(char *origin);

command_t ms_help = { "HELP", "Displays contextual help information.", AC_NONE, ms_cmd_help };

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
	command_add(&ms_help, ms_cmdtree);
}

void _moddeinit()
{
	command_delete(&ms_help, ms_cmdtree);
}

/* HELP <command> [params] */
void ms_cmd_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(memosvs.nick, origin, "***** \2%s Help\2 *****", memosvs.nick);
		notice(memosvs.nick, origin, "\2%s\2 allows users to send memos to registered users.", memosvs.nick);
		notice(memosvs.nick, origin, " ");
		notice(memosvs.nick, origin, "For more information on a command, type:");
		notice(memosvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", memosvs.disp);
		notice(memosvs.nick, origin, " ");

		command_help(memosvs.nick, origin, ms_cmdtree);

		notice(memosvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(memosvs.nick, origin, command, ms_help_commands)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(memosvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			notice(memosvs.nick, origin, "***** \2%s Help\2 *****", memosvs.nick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", memosvs.disp);

				if (buf[0])
					notice(memosvs.nick, origin, "%s", buf);
				else
					notice(memosvs.nick, origin, " ");
			}

			fclose(help_file);

			notice(memosvs.nick, origin, "***** \2End of Help\2 *****");
		}
		else
			notice(memosvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
