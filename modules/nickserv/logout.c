/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: logout.c 6631 2006-10-02 10:24:13Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/logout", FALSE, _modinit, _moddeinit,
	"$Id: logout.c 6631 2006-10-02 10:24:13Z jilles $",
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

	if ((!si->smu) && (!user || !pass))
	{
		command_fail(si, fault_authfail, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", user);
			return;
		}

#if 0
		if ((verify_password(mu, pass)) && (mu->user))
		{
			u = mu->user;
			notice(nicksvs.nick, u->nick, "You were logged out by \2%s\2.", si->su->nick);
		}
		else
		{
			command_fail(si, fault_authfail, "Authentication failed. Invalid password for \2%s\2.", mu->name);
			return;
		}
#endif
		/* remove this for now -- jilles */
		command_fail(si, fault_unimplemented, "External logout is not yet implemented.");
		return;
	}

	if (is_soper(si->smu))
		snoop("DESOPER: \2%s\2 as \2%s\2", get_oper_name(si), si->smu->name);

	if (si->su != u)
	{
		logcommand(si, CMDLOG_LOGIN, "LOGOUT %s", u->nick);
		command_success_nodata(si, "\2%s\2 has been logged out.", si->su->nick);
	}
	else
	{
		logcommand(si, CMDLOG_LOGIN, "LOGOUT");
		command_success_nodata(si, "You have been logged out.");
	}

	si->smu->lastlogin = CURRTIME;
	if (!ircd_on_logout(si->su->nick, si->smu->name, NULL))
	{
		LIST_FOREACH_SAFE(n, tn, si->smu->logins.head)
		{
			if (n->data == si->su)
			{
				node_del(n, &si->smu->logins);
				node_free(n);
				break;
			}
		}
		si->su->myuser = NULL;
	}
}
