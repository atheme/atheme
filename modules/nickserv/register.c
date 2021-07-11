/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the NickServ REGISTER function.
 */

#include <atheme.h>

static unsigned int ratelimit_count = 0;
static time_t ratelimit_firsttime = 0;

static void
ns_cmd_register(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct mynick *mn = NULL;
	mowgli_node_t *n;
	const char *account;
	const char *pass;
	const char *email;
	char lau[BUFSIZE], lao[BUFSIZE];
	struct hook_user_register_check hdata;
	struct hook_user_req req;

	if (si->smu)
	{
		command_fail(si, fault_already_authed, _("You are already logged in as \2%s\2."), entity(si->smu)->name);
		if (si->su != NULL && !mynick_find(si->su->nick) &&
				command_find(si->service->commands, "GROUP"))
			command_fail(si, fault_already_authed, _("Use %s to register %s to your account."), "GROUP", si->su->nick);
		return;
	}

	if (nicksvs.no_nick_ownership || si->su == NULL)
	{
		account = parv[0];
		pass = parv[1];
		email = parv[2];
	}
	else
	{
		account = si->su->nick;
		pass = parv[0];
		email = parv[1];
	}

	if (!account || !pass || !email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		if (nicksvs.no_nick_ownership || si->su == NULL)
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <account> <password> <email>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <password> <email>"));
		return;
	}

	if (strlen(pass) > PASSLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGISTER");
		command_fail(si, fault_badparams, _("Registration passwords may not be longer than \2%u\2 characters."), PASSLEN);
		return;
	}

	if (!nicksvs.no_nick_ownership && si->su == NULL && user_find_named(account))
	{
		command_fail(si, fault_noprivs, _("A user matching this account is already on IRC."));
		return;
	}

	if (!nicksvs.no_nick_ownership && IsDigit(*account))
	{
		command_fail(si, fault_badparams, _("For security reasons, you can't register your UID."));
		command_fail(si, fault_badparams, _("Please change to a real nickname, and try again."));
		return;
	}

	if (! is_valid_nick(account))
	{
		command_fail(si, fault_badparams, _("The account name \2%s\2 is invalid."), account);
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), email);
		return;
	}

	if (!email_within_limits(email))
	{
		command_fail(si, fault_toomany, _("\2%s\2 has too many accounts registered."), email);
		return;
	}

	if ((si->su != NULL && !strcasecmp(pass, si->su->nick)) || !strcasecmp(pass, account))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		if (nicksvs.no_nick_ownership || si->su == NULL)
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <account> <password> <email>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <password> <email>"));
		return;
	}

	// make sure it isn't registered already
	if (nicksvs.no_nick_ownership ? myuser_find(account) != NULL : mynick_find(account) != NULL)
	{
		command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered."), account);
		return;
	}

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
	{
		ratelimit_count = 0;
		ratelimit_firsttime = CURRTIME;
	}

	// Still do flood priv checking because the user may be in the ircop operclass
	if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("The system is currently too busy to process your registration, please try again later."));
		slog(LG_INFO, "NICKSERV:REGISTER:THROTTLED: \2%s\2 by \2%s\2", account, si->su != NULL ? si->su->nick : get_source_name(si));
		return;
	}

	hdata.si = si;
	hdata.account = account;
	hdata.email = email;
	hdata.password = pass;
	hdata.approved = 0;
	hook_call_user_can_register(&hdata);
	if (hdata.approved != 0)
		return;
	if (!nicksvs.no_nick_ownership)
	{
		hook_call_nick_can_register(&hdata);
		if (hdata.approved != 0)
			return;
	}

	if (si->su && !auth_module_loaded && !crypt_get_default_provider())
		(void) command_success_nodata(si, "%s", _("Warning: Your password will not be encrypted."));

	mu = myuser_add(account, auth_module_loaded ? "*" : pass, email, config_options.defuflags | MU_NOBURSTLOGIN | (auth_module_loaded ? MU_CRYPTPASS : 0));
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_add(mu, entity(mu)->name);
		mn->registered = CURRTIME;
		mn->lastseen = CURRTIME;
	}
	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;

	if (auth_module_loaded)
	{
		if (!verify_password(mu, pass))
		{
			command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), entity(mu)->name);
			bad_password(si, mu);
			atheme_object_unref(mu);
			return;
		}
	}

	if (me.auth == AUTH_EMAIL)
	{
		char *key = random_string(16);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, "private:verify:register:key", key);
		metadata_add(mu, "private:verify:register:timestamp", number_to_string(time(NULL)));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, mu, EMAIL_REGISTER, mu->email, key))
		{
			command_fail(si, fault_emailfail, _("Sending email failed, sorry! Registration aborted."));
			atheme_object_unref(mu);
			sfree(key);
			return;
		}

		command_success_nodata(si, _("An email containing nickname activation instructions has been sent to \2%s\2."), mu->email);
		command_success_nodata(si, _("Please check the address if you don't receive it. If it is incorrect, DROP then REGISTER again."));
		command_success_nodata(si, _("If you do not complete registration within one day, your nickname will expire."));

		sfree(key);
	}

	if (si->su != NULL)
	{
		si->su->myuser = mu;
		n = mowgli_node_create();
		mowgli_node_add(si->su, n, &mu->logins);

		if (!(mu->flags & MU_WAITAUTH))
			// only grant ircd registered status if it's verified
			ircd_on_login(si->su, mu, NULL);
	}

	command_add_flood(si, FLOOD_MODERATE);

	if (!nicksvs.no_nick_ownership && si->su != NULL)
		logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2 to \2%s\2", account, email);
	else
		logcommand(si, CMDLOG_REGISTER, "REGISTER: \2%s\2 to \2%s\2 by \2%s\2", account, email, si->su != NULL ? si->su->nick : get_source_name(si));

	if (is_soper(mu))
	{
		wallops("\2%s\2 registered the nick \2%s\2 and gained services operator privileges.", get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "SOPER: \2%s\2 as \2%s\2", get_oper_name(si), entity(mu)->name);
	}

	command_success_nodata(si, _("\2%s\2 is now registered to \2%s\2."), entity(mu)->name, mu->email);
	hook_call_user_register(mu);

	if (si->su != NULL)
	{
		snprintf(lau, BUFSIZE, "%s@%s", si->su->user, si->su->vhost);
		metadata_add(mu, "private:host:vhost", lau);

		snprintf(lao, BUFSIZE, "%s@%s", si->su->user, si->su->host);
		metadata_add(mu, "private:host:actual", lao);
	}

	if (!(mu->flags & MU_WAITAUTH))
	{
		req.si = si;
		req.mu = mu;
		req.mn = mn;
		hook_call_user_verify_register(&req);
	}
}

static struct command ns_register = {
	.name           = "REGISTER",
	.desc           = N_("Registers a nickname."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &ns_cmd_register,
	.help           = { .path = "nickserv/register" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_register);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_register);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/register", MODULE_UNLOAD_CAPABILITY_OK)
