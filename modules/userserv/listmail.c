/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LISTMAIL function.
 *
 * $Id: listmail.c 2359 2005-09-25 02:49:10Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/listmail", FALSE, _modinit, _moddeinit,
	"$Id: listmail.c 2359 2005-09-25 02:49:10Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_listmail(char *origin);

command_t us_listmail = { "LISTMAIL", "Lists nicknames registered to an e-mail address.", AC_IRCOP, us_cmd_listmail };

list_t *us_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_listmail, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_listmail, us_cmdtree);
}

static void us_cmd_listmail(char *origin)
{
	myuser_t *mu;
	node_t *n;
	char *email = strtok(NULL, " ");
	uint32_t i;
	uint32_t matches = 0;

	if (!email)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2LISTMAIL\2.");
		notice(usersvs.nick, origin, "Syntax: LISTMAIL <email>");
		return;
	}

	wallops("\2%s\2 is searching the nickname database for e-mail address \2%s\2", origin, email);

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

	if (matches == 0)
		notice(usersvs.nick, origin, "No nicknames matched e-mail address \2%s\2", email);
	else
		notice(usersvs.nick, origin, "\2%d\2 match%s for e-mail address \2%s\2", matches, matches != 1 ? "es" : "", email);
}
