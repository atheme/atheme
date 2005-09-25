/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 *
 * $Id: list.c 2359 2005-09-25 02:49:10Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 2359 2005-09-25 02:49:10Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_list(char *origin);

command_t us_list = { "LIST", "Lists nicknames registered matching a given pattern.", AC_IRCOP, us_cmd_list };

list_t *us_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_list, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_list, us_cmdtree);
}

static void us_cmd_list(char *origin)
{
	myuser_t *mu;
	node_t *n;
	char *nickpattern = strtok(NULL, " ");
	uint32_t i;
	uint32_t matches = 0;

	if (!nickpattern)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2LIST\2.");
		notice(usersvs.nick, origin, "Syntax: LIST <nickname pattern>");
		return;
	}

	wallops("\2%s\2 is searching the nickname database for nicknames matching \2%s\2", origin, nickpattern);

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
				if (metadata_find(mu, METADATA_USER, "private:alias:parent"))
				{
					notice(usersvs.nick, origin, "- %s (%s) [Child]", mu->name, mu->email);
				}
				else
				notice(usersvs.nick, origin, "- %s (%s)", mu->name, mu->email);
				matches++;
			}
		}
	}

	if (matches == 0)
		notice(usersvs.nick, origin, "No nicknames matched pattern \2%s\2", nickpattern);
	else
		notice(usersvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", nickpattern);
}
