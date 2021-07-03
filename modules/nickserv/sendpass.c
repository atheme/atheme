/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService SENDPASS function.
 */

#include <atheme.h>

enum specialoperation
{
	op_none,
	op_force,
	op_clear
};

static void
ns_cmd_sendpass(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	char *name = parv[0];
	char *newpass = NULL;
	char *key;
	struct metadata *md;
	enum specialoperation op = op_none;
	bool ismarked = false;
	char cmdtext[NICKLEN + 20];
	struct hook_user_needforce needforce_hdata;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SENDPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SENDPASS <account>"));
		return;
	}

	if (parc > 1)
	{
		if (!strcasecmp(parv[1], "FORCE"))
			op = op_force;
		else if (!strcasecmp(parv[1], "CLEAR"))
			op = op_clear;
		else
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SENDPASS");
			command_fail(si, fault_badparams, _("Syntax: SENDPASS <account> [FORCE|CLEAR]"));
			return;
		}
	}

	if (!(mu = myuser_find_by_nick(name)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (is SOPER)", name);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; you need %s privilege to send the password."), name, PRIV_ADMIN);
		return;
	}

	if (mu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not verified."), entity(mu)->name);
		return;
	}

	if ((md = metadata_find(mu, "private:mark:setter")))
	{
		ismarked = true;
		if (op == op_none)
		{
			logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (marked by \2%s\2)", entity(mu)->name, md->value);
			command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked by %s."), entity(mu)->name, md->value);
			if (has_priv(si, PRIV_MARK))
			{
				snprintf(cmdtext, sizeof cmdtext, "SENDPASS %s FORCE", entity(mu)->name);
				command_fail(si, fault_badparams, _("Use %s to override this restriction."), cmdtext);
			}
			return;
		}
		else if (!has_priv(si, PRIV_MARK))
		{
			logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (marked by \2%s\2)", entity(mu)->name, md->value);
			command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MARK);
			return;
		}
	}

	needforce_hdata.si = si;
	needforce_hdata.mu = mu;
	needforce_hdata.allowed = 1;

	hook_call_user_needforce(&needforce_hdata);

	if (!needforce_hdata.allowed)
	{
		ismarked = true;
		if (op == op_none)
		{
			logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (marked)", entity(mu)->name);
			command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked."), entity(mu)->name);
			if (has_priv(si, PRIV_MARK))
			{
				snprintf(cmdtext, sizeof cmdtext, "SENDPASS %s FORCE", entity(mu)->name);
				command_fail(si, fault_badparams, _("Use %s to override this restriction."), cmdtext);
			}
			return;
		}
		else if (!has_priv(si, PRIV_MARK))
		{
			logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (marked)", entity(mu)->name);
			command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MARK);
			return;
		}
	}

	if (op == op_clear)
	{
		if (metadata_find(mu, "private:setpass:key"))
		{
			metadata_delete(mu, "private:setpass:key");
			metadata_delete(mu, "private:sendpass:sender");
			metadata_delete(mu, "private:sendpass:timestamp");
			logcommand(si, CMDLOG_ADMIN, "SENDPASS:CLEAR: \2%s\2", entity(mu)->name);
			command_success_nodata(si, _("The password change key for \2%s\2 has been cleared."), entity(mu)->name);
		}
		else
			command_fail(si, fault_nochange, _("\2%s\2 did not have a password change key outstanding."), entity(mu)->name);
		return;
	}

	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		command_fail(si, fault_noprivs, _("This operation cannot be performed on %s, because someone is logged in to it."), entity(mu)->name);
		return;
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		command_fail(si, fault_noprivs, _("%s has been frozen by the %s administration."), entity(mu)->name, me.netname);
		return;
	}

	if (command_find(si->service->commands, "SETPASS"))
	{
		if (metadata_find(mu, "private:setpass:key"))
		{
			command_fail(si, fault_alreadyexists, _("\2%s\2 already has a password change key outstanding."), entity(mu)->name);
			command_fail(si, fault_alreadyexists, _("Use SENDPASS %s CLEAR to clear it so that a new one can be sent."), entity(mu)->name);
			return;
		}

		if (ismarked)
		{
			wallops("\2%s\2 sent the password for the \2MARKED\2 account %s.", get_oper_name(si), entity(mu)->name);
			if (md)
				command_success_nodata(si, _("Overriding MARK placed by %s on the account %s."), md->value, entity(mu)->name);
			else
				command_success_nodata(si, _("Overriding MARK on the account %s."), entity(mu)->name);
		}
		logcommand(si, CMDLOG_ADMIN, "SENDPASS: \2%s\2 (change key)", name);

		key = random_string(config_options.default_pass_length);

		const char *const hash = crypt_password(key);

		if (!hash)
		{
			command_fail(si, fault_internalerror, _("Hash generation for password change key failed."));
			sfree(key);
			return;
		}
		if (!sendemail(si->su != NULL ? si->su : si->service->me, mu, EMAIL_SETPASS, mu->email, key))
		{
			command_fail(si, fault_emailfail, _("Email send failed."));
			sfree(key);
			return;
		}

		metadata_add(mu, "private:sendpass:sender", get_oper_name(si));
		metadata_add(mu, "private:sendpass:timestamp", number_to_string(time(NULL)));
		metadata_add(mu, "private:setpass:key", hash);
		sfree(key);

		command_success_nodata(si, _("The password change key for \2%s\2 has been sent to \2%s\2."), entity(mu)->name, mu->email);
	}
	else {
		if (ismarked)
		{
			wallops("\2%s\2 sent the password for the \2MARKED\2 account %s.", get_oper_name(si), entity(mu)->name);
			if (md)
				command_success_nodata(si, _("Overriding MARK placed by %s on the account %s."), md->value, entity(mu)->name);
			else
				command_success_nodata(si, _("Overriding MARK on the account %s."), entity(mu)->name);
		}
		logcommand(si, CMDLOG_ADMIN, "SENDPASS: \2%s\2", name);

		newpass = random_string(16);
		metadata_add(mu, "private:sendpass:sender", get_oper_name(si));
		metadata_add(mu, "private:sendpass:timestamp", number_to_string(time(NULL)));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, mu, EMAIL_SENDPASS, mu->email, newpass))
		{
			command_fail(si, fault_emailfail, _("Email send failed."));
			sfree(newpass);
			return;
		}

		set_password(mu, newpass);
		sfree(newpass);

		command_success_nodata(si, _("The password for \2%s\2 has been sent to \2%s\2."), entity(mu)->name, mu->email);

		if (mu->flags & MU_NOPASSWORD)
		{
			mu->flags &= ~MU_NOPASSWORD;
			command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOPASSWORD", entity(mu)->name);
		}
	}
}

static struct command ns_sendpass = {
	.name           = "SENDPASS",
	.desc           = N_("Email registration passwords."),
	.access         = PRIV_USER_SENDPASS,
	.maxparc        = 2,
	.cmd            = &ns_cmd_sendpass,
	.help           = { .path = "nickserv/sendpass" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_CONFLICT(m, "nickserv/sendpass_user")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_sendpass);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_sendpass);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/sendpass", MODULE_UNLOAD_CAPABILITY_OK)
