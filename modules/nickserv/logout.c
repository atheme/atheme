/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: logout.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/logout", FALSE, _modinit, _moddeinit,
	"$Id: logout.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_logout(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_logout = { "LOGOUT", "Logs your services session out.", AC_NONE, 2, ns_cmd_logout };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_logout, ns_cmdtree);
	help_addentry(ns_helptree, "LOGOUT", "help/nickserv/logout", NULL);
}

void _moddeinit()
{
	command_delete(&ns_logout, ns_cmdtree);
	help_delentry(ns_helptree, "LOGOUT");
}

static void ns_cmd_logout(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = si->su;
	node_t *n, *tn;
	char *user = parv[0];
	char *pass = parv[1];

	if ((!si->su->myuser) && (!user || !pass))
	{
		notice(nicksvs.nick, si->su->nick, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			notice(nicksvs.nick, si->su->nick, "\2%s\2 is not registered.", user);
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
		notice(nicksvs.nick, si->su->nick, "External logout is not yet implemented.");
		return;
	}

	if (is_soper(si->su->myuser))
		snoop("DESOPER: \2%s\2 as \2%s\2", si->su->nick, si->su->myuser->name);

	if (si->su != u)
	{
		logcommand(nicksvs.me, si->su, CMDLOG_LOGIN, "LOGOUT %s", u->nick);
		notice(nicksvs.nick, si->su->nick, "\2%s\2 has been logged out.", si->su->nick);
	}
	else
	{
		logcommand(nicksvs.me, si->su, CMDLOG_LOGIN, "LOGOUT");
		notice(nicksvs.nick, si->su->nick, "You have been logged out.");
	}

	si->su->myuser->lastlogin = CURRTIME;
	if (!ircd_on_logout(si->su->nick, si->su->myuser->name, NULL))
	{
		LIST_FOREACH_SAFE(n, tn, si->su->myuser->logins.head)
		{
			if (n->data == si->su)
			{
				node_del(n, &si->su->myuser->logins);
				node_free(n);
				break;
			}
		}
		si->su->myuser = NULL;
	}
}
