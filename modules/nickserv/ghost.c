/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ GHOST function.
 *
 * $Id: ghost.c 3583 2005-11-06 21:48:28Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/ghost", FALSE, _modinit, _moddeinit,
	"$Id: ghost.c 3583 2005-11-06 21:48:28Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_ghost(char *origin);

command_t ns_ghost = { "GHOST", "Reclaims use of a nickname.", AC_NONE, ns_cmd_ghost };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_ghost, ns_cmdtree);
	help_addentry(ns_helptree, "GHOST", "help/nickserv/ghost", NULL);
}

void _moddeinit()
{
	command_delete(&ns_ghost, ns_cmdtree);
	help_delentry(ns_helptree, "GHOST");
}

void ns_cmd_ghost(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *target = strtok(NULL, " ");
	char *password = strtok(NULL, " ");
	user_t *target_u;
	node_t *n, *tn;

	if (!target)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2GHOST\2.");
		notice(nicksvs.nick, origin, "Syntax: GHOST <target> [password]");
		return;
	}

	mu = myuser_find(target);
	if (!mu)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	target_u = user_find(target);
	if (!target_u)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not online.", target);
		return;
	}
	else if (target_u == u)
	{
		notice(nicksvs.nick, origin, "You may not ghost yourself.");
		return;
	}

	if (mu == u->myuser || (password && !strcmp(password, mu->pass)))
	{
		snoop("GHOST: \2%s\2 by \2%s\2", mu->name, u->nick);

		skill(nicksvs.nick, target, "GHOST command used by %s!%s@%s", u->nick, u->user, u->vhost);
		user_delete(target);

		logcommand(nicksvs.me, u, CMDLOG_DO, "GHOST %s", target);

		notice(nicksvs.nick, origin, "\2%s\2 has been ghosted.", target);

		mu->lastlogin = CURRTIME;

		return;
	}

	snoop("GHOST:AF: \2%s\2 to \2%s\2", u->nick, mu->name);
	logcommand(nicksvs.me, u, CMDLOG_DO, "failed GHOST %s (bad password)", target);

	notice(nicksvs.nick, origin, "Invalid password for \2%s\2.", mu->name);
}
