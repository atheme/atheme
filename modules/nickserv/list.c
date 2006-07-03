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
	"nickserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_list(char *origin);

command_t ns_list = { "LIST", "Lists nicknames registered matching a given pattern.", PRIV_USER_AUSPEX, ns_cmd_list };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_list, ns_cmdtree);
	help_addentry(ns_helptree, "LIST", "help/nickserv/list", NULL);
}

void _moddeinit()
{
	command_delete(&ns_list, ns_cmdtree);
	help_delentry(ns_helptree, "LIST");
}

static void ns_cmd_list(char *origin)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	node_t *n;
	char *nickpattern = strtok(NULL, " ");
	char buf[BUFSIZE];
	uint32_t i;
	uint32_t matches = 0;

	if (u == NULL)
		return;

	if (!nickpattern)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LIST");
		notice(nicksvs.nick, origin, "Syntax: LIST <nickname pattern>");
		return;
	}

	wallops("\2%s\2 is searching the nickname database for nicknames matching \2%s\2", origin, nickpattern);
	notice(nicksvs.nick, origin, "Nicknames matching pattern \2%s\2:", nickpattern);


	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;


			if (!match(nickpattern, mu->name))
			{
				/* in the future we could add a LIMIT parameter */
				*buf = '\0';

				if (metadata_find(mu, METADATA_USER, "private:freeze:freezer")) {
					if (*buf)
						strlcat(buf, " ", BUFSIZE);

					strlcat(buf, "\2[frozen]\2", BUFSIZE);
				}
				if (metadata_find(mu, METADATA_USER, "private:mark:setter")) {
					if (*buf)
						strlcat(buf, " ", BUFSIZE);

					strlcat(buf, "\2[marked]\2", BUFSIZE);
				}
				
					notice(nicksvs.nick, origin, "- %s (%s) %s", mu->name, mu->email, buf);
					matches++;
			}
		}
	}

	logcommand(nicksvs.me, u, CMDLOG_ADMIN, "LIST %s (%d matches)", nickpattern, matches);
	if (matches == 0)
		notice(nicksvs.nick, origin, "No nicknames matched pattern \2%s\2", nickpattern);
	else
		notice(nicksvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", nickpattern);
}
