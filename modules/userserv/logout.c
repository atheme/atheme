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
	"userserv/logout", FALSE, _modinit, _moddeinit,
	"$Id: logout.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_logout(sourceinfo_t *si, int parc, char *parv[]);

command_t us_logout = { "LOGOUT", "Logs your services session out.", AC_NONE, 2, us_cmd_logout };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_logout, us_cmdtree);
	help_addentry(us_helptree, "LOGOUT", "help/userserv/logout", NULL);
}

void _moddeinit()
{
	command_delete(&us_logout, us_cmdtree);
	help_delentry(us_helptree, "LOGOUT");
}

static void us_cmd_logout(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = si->su;
	node_t *n, *tn;
	char *user = parv[0];
	char *pass = parv[1];

	if ((!si->su->myuser) && (!user || !pass))
	{
		notice(usersvs.nick, si->su->nick, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			notice(usersvs.nick, si->su->nick, "\2%s\2 is not registered.", user);
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
			notice(usersvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
			return;
		}
#endif
		/* remove this for now -- jilles */
		notice(usersvs.nick, si->su->nick, "External logout is not yet implemented.");
		return;
	}

	if (is_soper(si->su->myuser))
		snoop("DESOPER: \2%s\2 as \2%s\2", si->su->nick, si->su->myuser->name);

	if (si->su != u)
	{
		logcommand(usersvs.me, si->su, CMDLOG_LOGIN, "LOGOUT %s", u->nick);
		notice(usersvs.nick, si->su->nick, "\2%s\2 has been logged out.", si->su->nick);
	}
	else
	{
		logcommand(usersvs.me, si->su, CMDLOG_LOGIN, "LOGOUT");
		notice(usersvs.nick, si->su->nick, "You have been logged out.");
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
