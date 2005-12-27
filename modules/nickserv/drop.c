/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ DROP function.
 *
 * $Id: drop.c 4223 2005-12-27 19:30:05Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/drop", FALSE, _modinit, _moddeinit,
	"$Id: drop.c 4223 2005-12-27 19:30:05Z jilles $",
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
	user_t *u = user_find(origin);
	myuser_t *mu;
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

	if ((pass || !has_priv(u, PRIV_USER_ADMIN)) && !verify_password(mu, pass))
	{
		notice(nicksvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
		return;
	}

	if (is_sra(mu))
	{
		notice(nicksvs.nick, origin, "The nickname \2%s\2 belongs to a services root administrator; it cannot be dropped.", nick);
		return;
	}

	if (!pass)
		wallops("%s dropped the nickname \2%s\2", origin, mu->name);

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, u->nick);
	logcommand(nicksvs.me, u, pass ? CMDLOG_REGISTER : CMDLOG_ADMIN, "DROP %s%s", mu->name, pass ? "" : " (admin)");
	hook_call_event("user_drop", mu);
	notice(nicksvs.nick, origin, "The nickname \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu->name);
}
