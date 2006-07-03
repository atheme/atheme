/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 *
 * $Id: listmail.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/listmail", FALSE, _modinit, _moddeinit,
	"$Id: listmail.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_listmail(char *origin);

command_t us_listmail = { "LISTMAIL", "Lists accounts registered to an e-mail address.", PRIV_USER_AUSPEX, us_cmd_listmail };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_listmail, us_cmdtree);
	help_addentry(us_helptree, "LISTMAIL", "help/userserv/listmail", NULL);
}

void _moddeinit()
{
	command_delete(&us_listmail, us_cmdtree);
	help_delentry(us_helptree, "LISTMAIL");
}

static void us_cmd_listmail(char *origin)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	node_t *n;
	char *email = strtok(NULL, " ");
	uint32_t i;
	uint32_t matches = 0;

	if (u == NULL)
		return;

	if (!email)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "LISTMAIL");
		notice(usersvs.nick, origin, "Syntax: LISTMAIL <email>");
		return;
	}

	wallops("\2%s\2 is searching the account database for e-mail address \2%s\2", origin, email);

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			if (!match(email, mu->email))
			{
				/* in the future we could add a LIMIT parameter */
				if (matches == 0)
					notice(usersvs.nick, origin, "Nicknames matching e-mail address \2%s\2:", email);
				notice(usersvs.nick, origin, "- %s (%s)", mu->name, mu->email);
				matches++;
			}
		}
	}

	logcommand(usersvs.me, u, CMDLOG_ADMIN, "LISTMAIL %s (%d matches)", email, matches);
	if (matches == 0)
		notice(usersvs.nick, origin, "No accounts matched e-mail address \2%s\2", email);
	else
		notice(usersvs.nick, origin, "\2%d\2 match%s for e-mail address \2%s\2", matches, matches != 1 ? "es" : "", email);
}
