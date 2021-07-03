/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Patrick Fish, et al.
 *
 * This file contains code for nickserv RESETPASS
 */

#include <atheme.h>

static void
ns_cmd_resetpass(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct metadata *md;
	char *name = parv[0];
	char *newpass;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "RESETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: RESETPASS <account>"));
		return;
	}

	if (!(mu = myuser_find_by_nick(name)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RESETPASS \2%s\2 (is SOPER)", name);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; you need %s privilege to reset the password."), name, PRIV_ADMIN);
		return;
	}

	if ((md = metadata_find(mu, "private:mark:setter")))
	{
		if (has_priv(si, PRIV_MARK))
		{
			wallops("\2%s\2 reset the password for the \2MARKED\2 account %s.", get_oper_name(si), entity(mu)->name);
			logcommand(si, CMDLOG_ADMIN, "RESETPASS: \2%s\2 (overriding mark by \2%s\2)", entity(mu)->name, md->value);
			command_success_nodata(si, _("Overriding MARK placed by %s on the account %s."), md->value, entity(mu)->name);
		}
		else
		{
			logcommand(si, CMDLOG_ADMIN, "failed RESETPASS \2%s\2 (marked by \2%s\2)", entity(mu)->name, md->value);
			command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked by %s."), entity(mu)->name, md->value);
			return;
		}
	}
	else
	{
		wallops("\2%s\2 reset the password for the account %s", get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "RESETPASS: \2%s\2", entity(mu)->name);
	}

	newpass = random_string(config_options.default_pass_length);
	metadata_delete(mu, "private:setpass:key");
	metadata_add(mu, "private:sendpass:sender", get_oper_name(si));
	metadata_add(mu, "private:sendpass:timestamp", number_to_string(time(NULL)));
	set_password(mu, newpass);
	command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2."), entity(mu)->name, newpass);

	sfree(newpass);

	if (mu->flags & MU_NOPASSWORD)
	{
		mu->flags &= ~MU_NOPASSWORD;
		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOPASSWORD", entity(mu)->name);
	}
}

static struct command ns_resetpass = {
	.name           = "RESETPASS",
	.desc           = N_("Resets an account password."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 1,
	.cmd            = &ns_cmd_resetpass,
	.help           = { .path = "nickserv/resetpass" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_resetpass);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_resetpass);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/resetpass", MODULE_UNLOAD_CAPABILITY_OK)
