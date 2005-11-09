/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: logout.c 3741 2005-11-09 13:02:50Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/logout", FALSE, _modinit, _moddeinit,
	"$Id: logout.c 3741 2005-11-09 13:02:50Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_logout(char *origin);

command_t ns_logout = { "LOGOUT", "Logs your services session out.",
                        AC_NONE, ns_cmd_logout };
                                                                                   
list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
        command_add(&ns_logout, ns_cmdtree);
	help_addentry(ns_helptree, "LOGOUT", "help/nickserv/logout", NULL);
}

void _moddeinit()
{
	command_delete(&ns_logout, ns_cmdtree);
	help_delentry(ns_helptree, "LOGOUT");
}

static void ns_cmd_logout(char *origin)
{
	user_t *u = user_find(origin);
	user_t *source = u;
	node_t *n, *tn;
	char *user = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if ((!u->myuser) && (!user || !pass))
	{
		notice(nicksvs.nick, origin, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not registered.", user);
			return;
		}

#if 0
		if ((verify_password(mu, pass)) && (mu->user))
		{
			u = mu->user;
			notice(u->nick, "You were logged out by \2%s\2.", origin);
		}
		else
		{
			notice(nicksvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
			return;
		}
#endif
		/* remove this for now -- jilles */
		notice(nicksvs.nick, origin, "External logout is not yet implemented.");
		return;
	}

	if (is_sra(u->myuser))
		snoop("DESRA: \2%s\2 as \2%s\2", u->nick, u->myuser->name);

	/*snoop("LOGOUT: \2%s\2 from \2%s\2", u->nick, u->myuser->name);*/

	if (irccasecmp(origin, u->nick))
	{
		logcommand(nicksvs.me, source, CMDLOG_LOGIN, "LOGOUT %s", u->nick);
		notice(nicksvs.nick, origin, "\2%s\2 has been logged out.", u->nick);
	}
	else
	{
		logcommand(nicksvs.me, source, CMDLOG_LOGIN, "LOGOUT");
		notice(nicksvs.nick, origin, "You have been logged out.");
	}

	ircd_on_logout(u->nick, u->myuser->name, NULL);

	u->myuser->lastlogin = CURRTIME;
	LIST_FOREACH_SAFE(n, tn, u->myuser->logins.head)
	{
		if (n->data == u)
		{
			node_del(n, &u->myuser->logins);
			node_free(n);
			break;
		}
	}
	u->myuser = NULL;
}
