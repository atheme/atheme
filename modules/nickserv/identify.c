/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 William Pitcock, et al.
 *
 * This file contains code for the NickServ IDENTIFY and LOGIN functions.
 */

#include <atheme.h>

// Check whether we are compiling IDENTIFY or LOGIN
#ifdef NICKSERV_LOGIN
#define COMMAND_UC      "LOGIN"
#define COMMAND_LC      "login"
#define COMMAND_DESC    N_("Authenticates to a services account.")
#else
#define COMMAND_UC      "IDENTIFY"
#define COMMAND_LC      "identify"
#define COMMAND_DESC	N_("Identifies to services for a nickname.")
#endif

static void
ns_cmd_login(struct sourceinfo *si, int parc, char *parv[])
{
	struct user *u = si->su;
	struct myuser *mu;
	mowgli_node_t *n, *tn;
	const char *target = parv[0];
	const char *password = parv[1];
	char lau[BUFSIZE];

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, COMMAND_UC);
		return;
	}

	if (u->flags & UF_DOING_SASL)
	{
		command_fail(si, fault_authfail, _("You are already performing an SASL login."));
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

	mu = myuser_find_by_nick(target);
	if (!mu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}

	struct hook_user_login_check login_req = {
		.si      = si,
		.mu      = mu,
		.allowed = true,
	};

	struct hook_user_logout_check logout_req = {
		.si      = si,
		.u       = u,
		.allowed = true,
		.relogin = true,
	};

	hook_call_user_can_login(&login_req);

	if (login_req.allowed && u->myuser)
		hook_call_user_can_logout(&logout_req);

	if (!login_req.allowed || !logout_req.allowed)
	{
		command_fail(si, fault_authfail, nicksvs.no_nick_ownership ? _("You cannot log in as \2%s\2 because the server configuration disallows it.")
									   : _("You cannot identify to \2%s\2 because the server configuration disallows it."), entity(mu)->name);
		logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (denied by hook)", entity(mu)->name);
		return;
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		command_fail(si, fault_authfail, nicksvs.no_nick_ownership ? _("You cannot log in as \2%s\2 because the account has been frozen.")
									   : _("You cannot identify to \2%s\2 because the nickname has been frozen."), entity(mu)->name);
		logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (frozen)", entity(mu)->name);
		return;
	}

	if (mu->flags & MU_NOPASSWORD)
	{
		command_fail(si, fault_authfail, _("Password authentication is disabled for this account."));
		logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (password authentication disabled)", entity(mu)->name);
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
		if (user_loginmaxed(mu))
		{
			command_fail(si, fault_toomany, _("There are already \2%zu\2 sessions logged in to \2%s\2 (maximum allowed: %u)."), MOWGLI_LIST_LENGTH(&mu->logins), entity(mu)->name, me.maxlogins);
			lau[0] = '\0';
			MOWGLI_ITER_FOREACH(n, mu->logins.head)
			{
				if (lau[0] != '\0')
					mowgli_strlcat(lau, ", ", sizeof lau);
				mowgli_strlcat(lau, ((struct user *)n->data)->nick, sizeof lau);
			}
			command_fail(si, fault_toomany, _("Logged in nicks are: %s"), lau);
			logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (too many logins)", entity(mu)->name);
			return;
		}

		// if they are identified to another account, nuke their session first
		if (u->myuser)
		{
			command_success_nodata(si, _("You have been logged out of \2%s\2."), entity(u->myuser)->name);

			if (ircd_on_logout(u, entity(u->myuser)->name))
				// logout killed the user...
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

		if (!(mu->flags & MU_CRYPTPASS))
			(void) command_success_nodata(si, _("Warning: Your password is not encrypted."));

		myuser_login(si->service, u, mu, true);
		logcommand(si, CMDLOG_LOGIN, COMMAND_UC);

		return;
	}

	logcommand(si, CMDLOG_LOGIN, "failed " COMMAND_UC " to \2%s\2 (bad password)", entity(mu)->name);

	command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), entity(mu)->name);
	bad_password(si, mu);
}

static struct command ns_login = {
	.name           = COMMAND_UC,
	.desc           = COMMAND_DESC,
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ns_cmd_login,
	.help           = { .path = "nickserv/" COMMAND_LC },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_login);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_login);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/" COMMAND_LC, MODULE_UNLOAD_CAPABILITY_OK)
