/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ GHOST function.
 *
 * $Id: ghost.c 5077 2006-04-14 11:45:22Z w00t $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/ghost", FALSE, _modinit, _moddeinit,
	"$Id: ghost.c 5077 2006-04-14 11:45:22Z w00t $",
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
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	char *target = strtok(NULL, " ");
	char *password = strtok(NULL, " ");
	user_t *target_u;

	if (!target)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "GHOST");
		notice(nicksvs.nick, origin, "Syntax: GHOST <target> [password]");
		return;
	}

	mu = myuser_find(target);
	target_u = user_find_named(target);
	if (!mu && !target_u)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

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

	if ((target_u->myuser && target_u->myuser == u->myuser) || /* they're identified under our account */
			(mu && mu == u->myuser) || /* we're identified under their account */
			(password && mu && verify_password(mu, password))) /* we have their password */
	{
		/* If we're ghosting an unregistered nick, mu will be unset,
		 * however if it _is_ registered, we still need to set it or
		 * the wrong user will have their last seen time updated... */
		if(target_u->myuser && target_u->myuser == u->myuser)
			mu = target_u->myuser;

		skill(nicksvs.nick, target, "GHOST command used by %s!%s@%s", u->nick, u->user, u->vhost);
		user_delete(target_u);

		logcommand(nicksvs.me, u, CMDLOG_DO, "GHOST %s", target);

		notice(nicksvs.nick, origin, "\2%s\2 has been ghosted.", target);

		mu->lastlogin = CURRTIME;

		return;
	}

	logcommand(nicksvs.me, u, CMDLOG_DO, "failed GHOST %s (bad password)", target);

	notice(nicksvs.nick, origin, "Invalid password for \2%s\2.", mu->name);
}
