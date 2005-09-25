/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: logout.c 2359 2005-09-25 02:49:10Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/logout", FALSE, _modinit, _moddeinit,
	"$Id: logout.c 2359 2005-09-25 02:49:10Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_logout(char *origin);

command_t us_logout = { "LOGOUT", "Logs your services session out.",
                        AC_NONE, us_cmd_logout };
                                                                                   
list_t *us_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
        command_add(&us_logout, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_logout, us_cmdtree);
}

static void us_cmd_logout(char *origin)
{
	user_t *u = user_find(origin);
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
		if ((!strcmp(mu->pass, pass)) && (mu->user))
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

	if (is_sra(u->myuser))
		snoop("DESRA: \2%s\2 as \2%s\2", u->nick, u->myuser->name);

	snoop("LOGOUT: \2%s\2 from \2%s\2", u->nick, u->myuser->name);

	if (irccasecmp(origin, u->nick))
		notice(usersvs.nick, origin, "\2%s\2 has been logged out.", u->nick);
	else
		notice(usersvs.nick, origin, "You have been logged out.");

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
