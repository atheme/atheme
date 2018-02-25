/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 */

#include "atheme.h"

static void ns_cmd_logout(struct sourceinfo *si, int parc, char *parv[]);

static struct command ns_logout = { "LOGOUT", N_("Logs your services session out."), AC_NONE, 2, ns_cmd_logout, { .path = "nickserv/logout" } };

static void
mod_init(struct module *const restrict m)
{
	service_named_bind_command("nickserv", &ns_logout);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_logout);
}

static void
ns_cmd_logout(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u = si->su;
	mowgli_node_t *n, *tn;
	struct mynick *mn;
	char *user = parv[0];
	char *pass = parv[1];

	if ((!si->smu) && (!user || !pass))
	{
		command_fail(si, fault_nochange, _("You are not logged in."));
		return;
	}

	if (user)
	{
		u = user_find(user);

		if (!u)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), user);
			return;
		}

		if (!u->myuser)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not logged in."), user);
			return;
		}

		if (u->myuser == si->smu || (pass != NULL && verify_password(u->myuser, pass)))
			notice(nicksvs.nick, u->nick, "You were logged out by \2%s\2.", si->su->nick);
		else if (pass != NULL)
		{
			logcommand(si, CMDLOG_LOGIN, "failed LOGOUT \2%s\2 (bad password)", u->nick);
			command_fail(si, fault_authfail, _("Authentication failed. Invalid password for \2%s\2."), entity(u->myuser)->name);
			bad_password(si, u->myuser);
			return;
		}
		else
		{
			command_fail(si, fault_authfail, _("You may not log out \2%s\2."), u->nick);
			return;
		}
	}
	else if (si->su == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOGOUT");
		command_fail(si, fault_needmoreparams, _("Syntax: LOGOUT <target> <password>"));
		return;
	}


	if (is_soper(u->myuser))
		logcommand(si, CMDLOG_ADMIN, "DESOPER: \2%s\2 as \2%s\2", u->nick, entity(u->myuser)->name);

	if (si->su != u)
	{
		logcommand(si, CMDLOG_LOGIN, "LOGOUT: \2%s\2", u->nick);
		command_success_nodata(si, _("\2%s\2 has been logged out."), u->nick);
	}
	else
	{
		logcommand(si, CMDLOG_LOGIN, "LOGOUT");
		command_success_nodata(si, _("You have been logged out."));
	}

	u->myuser->lastlogin = CURRTIME;
	mn = mynick_find(u->nick);
	if (mn != NULL && mn->owner == u->myuser)
		mn->lastseen = CURRTIME;

	if (!ircd_on_logout(u, entity(u->myuser)->name))
	{
		MOWGLI_ITER_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		{
			if (n->data == u)
			{
				mowgli_node_delete(n, &u->myuser->logins);
				mowgli_node_free(n);
				break;
			}
		}
		u->myuser = NULL;
	}
}

SIMPLE_DECLARE_MODULE_V1("nickserv/logout", MODULE_UNLOAD_CAPABILITY_OK)
