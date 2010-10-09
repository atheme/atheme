/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ IDENTIFY and LOGIN functions.
 *
 */

#include "atheme.h"

/* Check whether we are compiling IDENTIFY or LOGIN */
#ifdef NICKSERV_LOGIN
#define COMMAND_UC "LOGIN"
#define COMMAND_LC "login"
#else
#define COMMAND_UC "IDENTIFY"
#define COMMAND_LC "identify"
#endif

DECLARE_MODULE_V1
(
	"nickserv/" COMMAND_LC, false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_login(sourceinfo_t *si, int parc, char *parv[]);

#ifdef NICKSERV_LOGIN
command_t ns_login = { "LOGIN", N_("Authenticates to a services account."), AC_NONE, 2, ns_cmd_login, { .path = "nickserv/login" } };
#else
command_t ns_identify = { "IDENTIFY", N_("Identifies to services for a nickname."), AC_NONE, 2, ns_cmd_login, { .path = "nickserv/identify" } };
#endif

void _modinit(module_t *m)
{
#ifdef NICKSERV_LOGIN
	service_named_bind_command("nickserv", &ns_login);
#else
	service_named_bind_command("nickserv", &ns_identify);
#endif
}

void _moddeinit()
{
#ifdef NICKSERV_LOGIN
	service_named_unbind_command("nickserv", &ns_login);
#else
	service_named_unbind_command("nickserv", &ns_identify);
#endif
}

static void ns_cmd_login(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = si->su;
	myuser_t *mu;
	mynick_t *mn;
	mowgli_node_t *n, *tn;
	char *target = parv[0];
	char *password = parv[1];
	char lau[BUFSIZE];

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), COMMAND_UC);
		return;
	}

#ifndef NICKSERV_LOGIN
	if (!nicksvs.no_nick_ownership && target && !password)
	{
		password = target;
		target = si->su->nick;
	}
#endif

	if (!target || !password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, COMMAND_UC);
		command_fail(si, fault_needmoreparams, nicksvs.no_nick_ownership ? "Syntax: " COMMAND_UC " <account> <password>" : "Syntax: " COMMAND_UC " [nick] <password>");
		return;
	}

	if (nicksvs.no_nick_ownership)
		mu = myuser_find(target);
	else
	{
		mn = mynick_find(target);
		mu = mn != NULL ? mn->owner : NULL;
	}

	if (!mu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		command_fail(si, fault_authfail, nicksvs.no_nick_ownership ? "You cannot login as \2%s\2 because the account has been frozen." : "You cannot identify to \2%s\2 because the nickname has been frozen.", entity(mu)->name);
		logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (frozen)", entity(mu)->name);
		return;
	}

	if (u->myuser == mu)
	{
		command_fail(si, fault_nochange, _("You are already logged in as \2%s\2."), entity(u->myuser)->name);
		if (mu->flags & MU_WAITAUTH)
			command_fail(si, fault_nochange, _("Please check your email for instructions to complete your registration."));
		return;
	}
	else if (u->myuser != NULL && !command_find(si->service->commands, "LOGOUT"))
	{
		command_fail(si, fault_alreadyexists, _("You are already logged in as \2%s\2."), entity(u->myuser)->name);
		return;
	}

	if (verify_password(mu, password))
	{
		if (MOWGLI_LIST_LENGTH(&mu->logins) >= me.maxlogins)
		{
			command_fail(si, fault_toomany, _("There are already \2%zu\2 sessions logged in to \2%s\2 (maximum allowed: %u)."), MOWGLI_LIST_LENGTH(&mu->logins), entity(mu)->name, me.maxlogins);
			lau[0] = '\0';
			MOWGLI_ITER_FOREACH(n, mu->logins.head)
			{
				if (lau[0] != '\0')
					strlcat(lau, ", ", sizeof lau);
				strlcat(lau, ((user_t *)n->data)->nick, sizeof lau);
			}
			command_fail(si, fault_toomany, _("Logged in nicks are: %s"), lau);
			logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (too many logins)", entity(mu)->name);
			return;
		}

		/* if they are identified to another account, nuke their session first */
		if (u->myuser)
		{
			if (ircd_on_logout(u, entity(u->myuser)->name))
				/* logout killed the user... */
				return;
		        u->myuser->lastlogin = CURRTIME;
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

		command_success_nodata(si, nicksvs.no_nick_ownership ? _("You are now logged in as \2%s\2.") : _("You are now identified for \2%s\2."), entity(mu)->name);

		myuser_login(si->service, u, mu, true);
		logcommand(si, CMDLOG_LOGIN, COMMAND_UC);

		grant_channel_access(u, mu);

		return;
	}

	logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (bad password)", entity(mu)->name);

	command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), entity(mu)->name);
	bad_password(si, mu);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
