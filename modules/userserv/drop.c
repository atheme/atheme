/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the UserServ DROP function.
 *
 * $Id: drop.c 2819 2005-10-10 00:15:26Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 2819 2005-10-10 00:15:26Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_drop(char *origin);

command_t us_drop = { "DROP", "Drops a account registration.", AC_NONE, us_cmd_drop };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_drop, us_cmdtree);
	help_addentry(us_helptree, "DROP", "help/userserv/drop", NULL);
}

void _moddeinit()
{
	command_delete(&us_drop, us_cmdtree);
	help_delentry(us_helptree, "DROP");
}

static void us_cmd_drop(char *origin)
{
	uint32_t i;
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *tmc;
	node_t *n;
	char *acc = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if (!pass)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2DROP\2.");
		notice(usersvs.nick, origin, "Syntax: DROP <account> <password>");
		return;
	}

	if (!(mu = myuser_find(acc)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", acc);
		return;
	}

	if (!is_sra(u->myuser) && (!pass || strcmp(pass, mu->pass)))
	{
		notice(usersvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
		return;
	}

	if (is_sra(u->myuser) && !pass)
		wallops("%s used the \2DROP\2 cmd on the nickname %s", origin, mu->name);

	/* find all channels that are theirs and drop them */
	for (i = 1; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			tmc = (mychan_t *)n->data;
			if (tmc->founder == mu)
			{
				snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", tmc->name, u->nick, u->myuser->name);

				notice(usersvs.nick, origin, "The channel \2%s\2 has been dropped.", tmc->name);

				part(tmc->name, chansvs.nick);
				mychan_delete(tmc->name);
			}
		}
	}

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, u->nick);
	hook_call_event("user_drop", mu);
	ircd_on_logout(origin, u->myuser->name, NULL);
	if (u->myuser == mu)
		u->myuser = NULL;
	notice(usersvs.nick, origin, "The account \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu->name);
}
