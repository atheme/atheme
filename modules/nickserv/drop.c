/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ DROP function.
 *
 * $Id: drop.c 3633 2005-11-07 22:25:53Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 3633 2005-11-07 22:25:53Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_drop(char *origin);

command_t ns_drop = { "DROP", "Drops a nickname registration.", AC_NONE, ns_cmd_drop };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_drop, ns_cmdtree);
	help_addentry(ns_helptree, "DROP", "help/nickserv/drop", NULL);
}

void _moddeinit()
{
	command_add(&ns_drop, ns_cmdtree);
	help_delentry(ns_helptree, "DROP");
}

static void ns_cmd_drop(char *origin)
{
	uint32_t i;
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *tmc;
	node_t *n;
	char *nick = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if (!nick)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2DROP\2.");
		notice(nicksvs.nick, origin, "Syntax: DROP <nickname> <password>");
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", nick);
		return;
	}

	if (!is_sra(u->myuser) && (!pass || strcmp(pass, mu->pass)))
	{
		notice(nicksvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
		return;
	}

	if (is_sra(mu))
	{
		notice(nicksvs.nick, origin, "The nickname \2%s\2 belongs to a services root administrator; it cannot be dropped.", nick);
		return;
	}

	if (is_sra(u->myuser) && !pass)
		wallops("%s dropped the nickname \2%s\2", origin, mu->name);

	/* find all channels that are theirs and drop them */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			tmc = (mychan_t *)n->data;
			if ((tmc->founder == mu && tmc->successor == tmc->founder) ||
			    (tmc->founder == mu && tmc->successor == NULL))
			{
				snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", tmc->name, u->nick, u->myuser->name);

				notice(nicksvs.nick, origin, "The channel \2%s\2 has been dropped.", tmc->name);

				part(tmc->name, chansvs.nick);
				mychan_delete(tmc->name);
			}
			else if (tmc->founder == mu && tmc->successor != mu)
			{
				snoop("SUCCESSION: \2%s\2 -> \2%s\2", tmc->name, tmc->successor->name);
				chanacs_delete(tmc, tmc->successor, CA_SUCCESSOR);
				chanacs_add(tmc, tmc->successor, CA_FOUNDER);
				tmc->founder = tmc->successor;
				tmc->successor = NULL;

				myuser_notice(chansvs.nick, tmc->founder, "You are now the founder of \2%s\2 (as \2%s\2).",
					      tmc->name, tmc->founder->name);
			}
		}
	}

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, u->nick);
	logcommand(nicksvs.me, u, CMDLOG_REGISTER, "DROP %s", mu->name);
	hook_call_event("user_drop", mu);
	notice(nicksvs.nick, origin, "The nickname \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu->name);
}
