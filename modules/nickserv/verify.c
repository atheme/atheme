/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the NickServ VERIFY function.
 */

#include <atheme.h>

#define VERIFY_STR_THANKS   _("Thank you for verifying your e-mail address! You have taken steps in ensuring that your registrations are not exploited.")
#define VERIFY_STR_CONFIRM  _("\2%s\2 has now been verified.")

static void
ns_verify_activate_account(struct sourceinfo *si, struct myuser *mu, bool force)
{
	mowgli_node_t *n;
	struct hook_user_req req;

	mu->flags &= ~MU_WAITAUTH;

	metadata_delete(mu, "private:verify:register:key");
	metadata_delete(mu, "private:verify:register:timestamp");

	myuser_notice(nicksvs.nick, mu, VERIFY_STR_CONFIRM, entity(mu)->name);
	if (!force)
		myuser_notice(nicksvs.nick, mu, VERIFY_STR_THANKS);
	if (si->smu != mu) {
		command_success_nodata(si, VERIFY_STR_CONFIRM, entity(mu)->name);
		if (!force)
			command_success_nodata(si, VERIFY_STR_THANKS);
	}

	MOWGLI_ITER_FOREACH(n, mu->logins.head)
	{
		struct user *u = n->data;
		ircd_on_login(u, mu, NULL);
	}

	// XXX should this indeed be after ircd_on_login?
	req.si = si;
	req.mu = mu;
	req.mn = mynick_find(entity(mu)->name);
	hook_call_user_verify_register(&req);
}

static void
ns_cmd_verify(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct metadata *md;
	char *op = parv[0];
	char *nick = parv[1];
	char *key = parv[2];

	if (!op || !nick || !key)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VERIFY");
		command_fail(si, fault_needmoreparams, _("Syntax: VERIFY <operation> <account> <key>"));
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, nick);
		return;
	}

	if (!strcasecmp(op, "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !(md = metadata_find(mu, "private:verify:register:key")))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting verification."), nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			logcommand(si, CMDLOG_SET, "VERIFY:REGISTER: \2%s\2 (email: \2%s\2)", entity(mu)->name, mu->email);
			ns_verify_activate_account(si, mu, false);
			return;
		}

		logcommand(si, CMDLOG_SET, "failed VERIFY REGISTER \2%s\2, \2%s\2 (invalid key)", entity(mu)->name, mu->email);
		command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."),
			entity(mu)->name);

		return;
	}

	// forcing users to log in before we verify prevents some information leaks
	if (!(si->smu == mu))
	{
		command_fail(si, fault_badparams, _("Please log in before attempting to verify an email address change."));
		return;
	}

	if (!strcasecmp(op, "EMAILCHG"))
	{
		if (!(md = metadata_find(mu, "private:verify:emailchg:key")))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting verification."), nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			md = metadata_find(mu, "private:verify:emailchg:newemail");

			/* Make sure we reject "set email same@address" for
			 * several accounts (without verifying the change)
			 * followed by verifying all of them.
			 */
			if (!email_within_limits(md->value)) {
				command_fail(si, fault_toomany, _("\2%s\2 has too many accounts registered."), md->value);
				return;
			}

			myuser_set_email(mu, md->value);

			logcommand(si, CMDLOG_SET, "VERIFY:EMAILCHG: \2%s\2 (email: \2%s\2)", entity(mu)->name, mu->email);

			metadata_delete(mu, "private:verify:emailchg:key");
			metadata_delete(mu, "private:verify:emailchg:newemail");
			metadata_delete(mu, "private:verify:emailchg:timestamp");

			command_success_nodata(si, _("\2%s\2 has now been verified."), mu->email);

			if (metadata_find(mu, "private:verify:register:key"))
			{
				ns_verify_activate_account(si, mu, false);
			}

			return;
		}

		logcommand(si, CMDLOG_SET, "failed VERIFY EMAILCHG \2%s\2, \2%s\2 (invalid key)", entity(mu)->name, mu->email);
		command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."),
			entity(mu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid operation specified for \2VERIFY\2."));
		command_fail(si, fault_badparams, _("Please double-check your verification e-mail."));
		return;
	}
}

static void
ns_cmd_fverify(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *mu;
	struct metadata *md;
	char *op = parv[0];
	char *nick = parv[1];

	if (!op || !nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FVERIFY");
		command_fail(si, fault_needmoreparams, _("Syntax: FVERIFY <operation> <account>"));
		return;
	}

	if (!(mu = myuser_find_ext(nick)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, nick);
		return;
	}

	if (!strcasecmp(op, "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !metadata_find(mu, "private:verify:register:key"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting verification."), nick);
			return;
		}

		logcommand(si, CMDLOG_SET, "FVERIFY:REGISTER: \2%s\2 (email: \2%s\2)", entity(mu)->name, mu->email);
		ns_verify_activate_account(si, mu, true);

		return;
	}
	else if (!strcasecmp(op, "EMAILCHG"))
	{
		if (!metadata_find(mu, "private:verify:emailchg:key"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting verification."), nick);
			return;
		}

		md = metadata_find(mu, "private:verify:emailchg:newemail");

		myuser_set_email(mu, md->value);

		logcommand(si, CMDLOG_REGISTER, "FVERIFY:EMAILCHG: \2%s\2 (email: \2%s\2)", entity(mu)->name, mu->email);

		metadata_delete(mu, "private:verify:emailchg:key");
		metadata_delete(mu, "private:verify:emailchg:newemail");
		metadata_delete(mu, "private:verify:emailchg:timestamp");

		command_success_nodata(si, _("\2%s\2 has now been verified."), mu->email);

		if (metadata_find(mu, "private:verify:register:key"))
		{
			ns_verify_activate_account(si, mu, true);
		}

		return;
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid operation specified for \2FVERIFY\2."));
		command_fail(si, fault_badparams, _("Valid operations are REGISTER and EMAILCHG."));
		return;
	}
}

static struct command ns_verify = {
	.name           = "VERIFY",
	.desc           = N_("Verifies an account registration."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &ns_cmd_verify,
	.help           = { .path = "nickserv/verify" },
};

static struct command ns_fverify = {
	.name           = "FVERIFY",
	.desc           = N_("Forcefully verifies an account registration."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 2,
	.cmd            = &ns_cmd_fverify,
	.help           = { .path = "nickserv/fverify" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	service_named_bind_command("nickserv", &ns_verify);
	service_named_bind_command("nickserv", &ns_fverify);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_verify);
	service_named_unbind_command("nickserv", &ns_fverify);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/verify", MODULE_UNLOAD_CAPABILITY_OK)
