/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 *
 * $Id: list.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_list(char *origin);

command_t us_list = { "LIST", "Lists accounts registered matching a given pattern.", PRIV_USER_AUSPEX, us_cmd_list };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_list, us_cmdtree);
	help_addentry(us_helptree, "LIST", "help/userserv/list", NULL);
}

void _moddeinit()
{
	command_delete(&us_list, us_cmdtree);
	help_delentry(us_helptree, "LIST");
}

static void us_cmd_list(char *origin)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	node_t *n;
	char *nickpattern = strtok(NULL, " ");
	uint32_t i;
	uint32_t matches = 0;

	if (u == NULL)
		return;

	if (!nickpattern)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LIST");
		notice(usersvs.nick, origin, "Syntax: LIST <account pattern>");
		return;
	}

	wallops("\2%s\2 is searching the account database for accounts matching \2%s\2", origin, nickpattern);

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			if (!match(nickpattern, mu->name))
			{
				/* in the future we could add a LIMIT parameter */
				if (matches == 0)
					notice(usersvs.nick, origin, "Nicknames matching pattern \2%s\2:", nickpattern);
				notice(usersvs.nick, origin, "- %s (%s)", mu->name, mu->email);
				matches++;
			}
		}
	}

	logcommand(usersvs.me, u, CMDLOG_ADMIN, "LIST %s (%d matches)", nickpattern, matches);
	if (matches == 0)
		notice(usersvs.nick, origin, "No accounts matched pattern \2%s\2", nickpattern);
	else
		notice(usersvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", nickpattern);
}
