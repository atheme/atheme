/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2007 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the NickServ SETPASS function.
 */

#include <atheme.h>

static void
ns_cmd_setpass(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct metadata *md;
	char *nick = parv[0];
	char *key = parv[1];
	char *password = parv[2];

	if (!nick || !key || !password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (strchr(password, ' '))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, nick);
		return;
	}

	if (si->smu == mu)
	{
		command_fail(si, fault_already_authed, _("You are logged in and can change your password using the SET PASSWORD command."));
		return;
	}

	if (strlen(password) > PASSLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		command_fail(si, fault_badparams, _("Registration passwords may not be longer than \2%u\2 characters."), PASSLEN);
		return;
	}

	if (!strcasecmp(password, entity(mu)->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	md = metadata_find(mu, "private:setpass:key");
	if (md == NULL || crypt_verify_password(key, md->value, NULL) == NULL)
	{
		if (md != NULL)
			logcommand(si, CMDLOG_SET, "failed SETPASS (invalid key)");
		command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."), entity(mu)->name);
		return;
	}

	logcommand(si, CMDLOG_SET, "SETPASS: \2%s\2", entity(mu)->name);

	metadata_delete(mu, "private:setpass:key");
	metadata_delete(mu, "private:sendpass:sender");
	metadata_delete(mu, "private:sendpass:timestamp");

	set_password(mu, password);
	command_success_nodata(si, _("The password for \2%s\2 has been successfully changed."), entity(mu)->name);

	if (mu->flags & MU_NOPASSWORD)
	{
		mu->flags &= ~MU_NOPASSWORD;
		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOPASSWORD", entity(mu)->name);
	}
}

static void
clear_setpass_key(struct user *u)
{
	struct myuser *mu = u->myuser;

	if (!metadata_find(mu, "private:setpass:key"))
		return;

	metadata_delete(mu, "private:setpass:key");
	metadata_delete(mu, "private:sendpass:sender");
	metadata_delete(mu, "private:sendpass:timestamp");

	notice(nicksvs.nick, u->nick, "Warning: SENDPASS had been used to mail you a password recovery "
		"key. Since you have identified, that key is no longer valid.");
}

static void
show_setpass(struct hook_user_req *hdata)
{
	if (has_priv(hdata->si, PRIV_USER_AUSPEX))
	{
		if (metadata_find(hdata->mu, "private:setpass:key"))
			command_success_nodata(hdata->si, _("%s has an active password reset key"), entity(hdata->mu)->name);

		struct metadata *md;
		char strfbuf[BUFSIZE];

		if ((md = metadata_find(hdata->mu, "private:sendpass:sender")) != NULL)
		{
			const char *sender = md->value;
			time_t ts;
			struct tm *tm;

			md = metadata_find(hdata->mu, "private:sendpass:timestamp");
			ts = md != NULL ? atoi(md->value) : 0;

			tm = localtime(&ts);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

			command_success_nodata(hdata->si, _("%s was \2SENDPASSED\2 by %s on %s"), entity(hdata->mu)->name, sender, strfbuf);
		}
	}
}

static struct command ns_setpass = {
	.name           = "SETPASS",
	.desc           = N_("Changes a password using an authcode."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &ns_cmd_setpass,
	.help           = { .path = "nickserv/setpass" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	hook_add_user_identify(clear_setpass_key);
	hook_add_user_info(show_setpass);
	service_named_bind_command("nickserv", &ns_setpass);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	hook_del_user_identify(clear_setpass_key);
	hook_del_user_info(show_setpass);
	service_named_unbind_command("nickserv", &ns_setpass);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/setpass", MODULE_UNLOAD_CAPABILITY_OK)
