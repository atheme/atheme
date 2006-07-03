/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: logout.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/logout", FALSE, _modinit, _moddeinit,
	"$Id: logout.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_logout(char *origin);

command_t us_logout = { "LOGOUT", "Logs your services session out.",
                        AC_NONE, us_cmd_logout };
                                                                                   
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

static void us_cmd_logout(char *origin)
{
	user_t *u = user_find_named(origin);
	user_t *source = u;
	node_t *n, *tn;
	char *user = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if ((!u->myuser) && (!user || !pass))
	{
		notice(usersvs.nick, origin, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			notice(usersvs.nick, origin, "\2%s\2 is not registered.", user);
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
		notice(usersvs.nick, origin, "External logout is not yet implemented.");
		return;
	}

	if (is_soper(u->myuser))
		snoop("DESOPER: \2%s\2 as \2%s\2", u->nick, u->myuser->name);

	if (irccasecmp(origin, u->nick))
	{
		logcommand(usersvs.me, source, CMDLOG_LOGIN, "LOGOUT %s", u->nick);
		notice(usersvs.nick, origin, "\2%s\2 has been logged out.", u->nick);
	}
	else
	{
		logcommand(usersvs.me, source, CMDLOG_LOGIN, "LOGOUT");
		notice(usersvs.nick, origin, "You have been logged out.");
	}

	u->myuser->lastlogin = CURRTIME;
	if (!ircd_on_logout(u->nick, u->myuser->name, NULL))
	{
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
}
