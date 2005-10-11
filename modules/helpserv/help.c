/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the helpserv HELP command.
 *
 * $Id: help.c 2843 2005-10-11 12:44:34Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"helpserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 2843 2005-10-11 12:44:34Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *hs_cmdtree;
list_t *hs_helptree;

static void hs_cmd_help(char *origin);

command_t hs_help = { "HELP", "Displays contextual help information.", AC_NONE, hs_cmd_help };

void _modinit(module_t *m)
{
	hs_cmdtree = module_locate_symbol("helpserv/main", "hs_cmdtree");
	hs_helptree = module_locate_symbol("helpserv/main", "hs_helptree");

	command_add(&hs_help, hs_cmdtree);
	help_addentry(hs_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&hs_help, hs_cmdtree);
	help_delentry(hs_helptree, "HELP");
}

/* HELP <command> [params] */
void hs_cmd_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(helpsvs.nick, origin, "***** \2%s Help\2 *****", helpsvs.nick);
		notice(helpsvs.nick, origin, "\2%s\2 manages help requests for support staff.", helpsvs.nick);
		notice(helpsvs.nick, origin, " ");
		notice(helpsvs.nick, origin, "For more information on a command, type:");
		notice(helpsvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", helpsvs.disp);
		notice(helpsvs.nick, origin, " ");

		command_help(helpsvs.nick, origin, hs_cmdtree);

		notice(helpsvs.nick, origin, "***** \2End of Help\2 *****");
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(helpsvs.nick, origin, command, hs_helptree)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(helpsvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			notice(helpsvs.nick, origin, "***** \2%s Help\2 *****", helpsvs.nick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", helpsvs.disp);

				if (buf[0])
					notice(helpsvs.nick, origin, "%s", buf);
				else
					notice(helpsvs.nick, origin, " ");
			}

			fclose(help_file);

			notice(helpsvs.nick, origin, "***** \2End of Help\2 *****");
		}
		else if (c->func)
			c->func(origin);
		else
			notice(helpsvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
