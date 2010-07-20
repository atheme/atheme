/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/logout", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_logout(sourceinfo_t *si, int parc, char *parv[]);

cmd_t ns_logout = { N_("Logs your services session out."), AC_NONE, "help/nickserv/logout", 2, ns_cmd_logout };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	cmd_add("nickserv:logout", &ns_logout);
}

void _moddeinit()
{
	cmd_del("nickserv:logout");
}

static void ns_cmd_logout(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = si->su;
	node_t *n, *tn;
	mynick_t *mn;
	char *user = parv[0];
	char *pass = parv[1];

	if ((!si->smu) && (!user || !pass))
	{
		command_fail(si, fault_nochange, _("You are not logged in."));
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), user);
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
			command_fail(si, fault_authfail, _("Authentication failed. Invalid password for \2%s\2."), mu->name);
			bad_password(si, mu);
			return;
		}
#endif
		/* remove this for now -- jilles */
		command_fail(si, fault_unimplemented, _("External logout is not yet implemented."));
		return;
	}
	else if (si->su == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOGOUT");
		command_fail(si, fault_needmoreparams, _("Syntax: LOGOUT <target> <password>"));
		return;
	}


	if (is_soper(si->smu))
		logcommand(si, CMDLOG_ADMIN, "DESOPER: \2%s\2 as \2%s\2", si->su->nick, si->smu->name);

	if (si->su != u)
	{
		logcommand(si, CMDLOG_LOGIN, "LOGOUT: \2%s\2", u->nick);
		command_success_nodata(si, _("\2%s\2 has been logged out."), si->su->nick);
	}
	else
	{
		logcommand(si, CMDLOG_LOGIN, "LOGOUT");
		command_success_nodata(si, _("You have been logged out."));
	}

	si->smu->lastlogin = CURRTIME;
	if (si->su != NULL)
	{
		mn = mynick_find(si->su->nick);
		if (mn != NULL && mn->owner == si->smu)
			mn->lastseen = CURRTIME;
		if (!ircd_on_logout(si->su, si->smu->name))
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
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
