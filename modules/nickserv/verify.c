/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ VERIFY function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/verify", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_verify(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_fverify(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_verify = { "VERIFY", N_("Verifies an account registration."), AC_NONE, 3, ns_cmd_verify, { .path = "nickserv/verify" } };
command_t ns_fverify = { "FVERIFY", N_("Forcefully verifies an account registration."), PRIV_USER_ADMIN, 2, ns_cmd_fverify, { .path = "nickserv/fverify" } };

void _modinit(module_t *m)
{
	service_named_bind_command("nickserv", &ns_verify);
	service_named_bind_command("nickserv", &ns_fverify);
}

void _moddeinit()
{
	service_named_unbind_command("nickserv", &ns_verify);
	service_named_unbind_command("nickserv", &ns_fverify);
}

static void ns_cmd_verify(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	mowgli_node_t *n;
	char *op = parv[0];
	char *nick = parv[1];
	char *key = parv[2];
	hook_user_req_t req;

	if (!op || !nick || !key)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VERIFY");
		command_fail(si, fault_needmoreparams, _("Syntax: VERIFY <operation> <account> <key>"));
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), nick);
		return;
	}

	/* forcing users to log in before we verify
	 * prevents some information leaks
	 */
	if (!(si->smu == mu))
	{
		command_fail(si, fault_badparams, _("Please log in before attempting to verify your registration."));
		return;
	}

	if (!strcasecmp(op, "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !(md = metadata_find(mu, "private:verify:register:key")))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting authorization."), nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			mu->flags &= ~MU_WAITAUTH;

			logcommand(si, CMDLOG_SET, "VERIFY:REGISTER: \2%s\2 (email: \2%s\2)", get_source_name(si), mu->email);

			metadata_delete(mu, "private:verify:register:key");
			metadata_delete(mu, "private:verify:register:timestamp");

			command_success_nodata(si, _("\2%s\2 has now been verified."), entity(mu)->name);
			command_success_nodata(si, _("Thank you for verifying your e-mail address! You have taken steps in ensuring that your registrations are not exploited."));
			MOWGLI_ITER_FOREACH(n, mu->logins.head)
			{
				user_t *u = n->data;
				ircd_on_login(u, mu, NULL);
			}

			/* XXX should this indeed be after ircd_on_login? */
			req.si = si;
			req.mu = mu;
			req.mn = mynick_find(entity(mu)->name);
			hook_call_user_verify_register(&req);

			return;
		}

		logcommand(si, CMDLOG_SET, "failed VERIFY REGISTER \2%s\2, \2%s\2 (invalid key)", get_source_name(si), mu->email);
		command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."), 
			entity(mu)->name);

		return;
	}
	else if (!strcasecmp(op, "EMAILCHG"))
	{
		if (!(md = metadata_find(mu, "private:verify:emailchg:key")))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting authorization."), nick);
			return;
		}

		if (!strcasecmp(key, md->value))
		{
			md = metadata_find(mu, "private:verify:emailchg:newemail");

			myuser_set_email(mu, md->value);

			logcommand(si, CMDLOG_SET, "VERIFY:EMAILCHG: \2%s\2 (email: \2%s\2)", get_source_name(si), mu->email);

			metadata_delete(mu, "private:verify:emailchg:key");
			metadata_delete(mu, "private:verify:emailchg:newemail");
			metadata_delete(mu, "private:verify:emailchg:timestamp");

			command_success_nodata(si, _("\2%s\2 has now been verified."), mu->email);

			return;
		}

		logcommand(si, CMDLOG_SET, "failed VERIFY EMAILCHG \2%s\2, \2%s\2 (invalid key)", get_source_name(si), mu->email);
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

static void ns_cmd_fverify(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	mowgli_node_t *n;
	char *op = parv[0];
	char *nick = parv[1];
	hook_user_req_t req;

	if (!op || !nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FVERIFY");
		command_fail(si, fault_needmoreparams, _("Syntax: FVERIFY <operation> <account>"));
		return;
	}

	if (!(mu = myuser_find_ext(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), nick);
		return;
	}

	if (!strcasecmp(op, "REGISTER"))
	{
		if (!(mu->flags & MU_WAITAUTH) || !metadata_find(mu, "private:verify:register:key"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting authorization."), nick);
			return;
		}

		mu->flags &= ~MU_WAITAUTH;

		logcommand(si, CMDLOG_REGISTER, "FVERIFY:REGISTER: \2%s\2 (email: \2%s\2)", entity(mu)->name, mu->email);

		metadata_delete(mu, "private:verify:register:key");
		metadata_delete(mu, "private:verify:register:timestamp");

		command_success_nodata(si, _("\2%s\2 has now been verified."), entity(mu)->name);
		MOWGLI_ITER_FOREACH(n, mu->logins.head)
		{
			user_t *u = n->data;
			ircd_on_login(u, mu, NULL);
		}

		/* XXX should this indeed be after ircd_on_login? */
		req.si = si;
		req.mu = mu;
		req.mn = mynick_find(entity(mu)->name);
		hook_call_user_verify_register(&req);

		return;
	}
	else if (!strcasecmp(op, "EMAILCHG"))
	{
		if (!metadata_find(mu, "private:verify:emailchg:key"))
		{
			command_fail(si, fault_badparams, _("\2%s\2 is not awaiting authorization."), nick);
			return;
		}

		md = metadata_find(mu, "private:verify:emailchg:newemail");

		myuser_set_email(mu, md->value);

		logcommand(si, CMDLOG_REGISTER, "FVERIFY:EMAILCHG: \2%s\2 (email: \2%s\2)", entity(mu)->name, mu->email);

		metadata_delete(mu, "private:verify:emailchg:key");
		metadata_delete(mu, "private:verify:emailchg:newemail");
		metadata_delete(mu, "private:verify:emailchg:timestamp");

		command_success_nodata(si, _("\2%s\2 has now been verified."), mu->email);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid operation specified for \2FVERIFY\2."));
		command_fail(si, fault_badparams, _("Valid operations are REGISTER and EMAILCHG."));
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
