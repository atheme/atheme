/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2021 Atheme Development Group (https://atheme.github.io/)
 *
 * Returns the canonicalized form of an email address.
 */

#include <atheme.h>

static void
ns_cmd_canonmail(struct sourceinfo *si, int parc, char *parv[])
{
	char *email = parv[0];

	if (!email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CANONMAIL");
		command_fail(si, fault_needmoreparams, _("Usage: CANONMAIL <email>"));
		return;
	}

	stringref email_canonical = canonicalize_email(email);

	command_success_string(si, email_canonical, _("Email address \2%s\2 canonicalizes to \2%s\2"), email, email_canonical);

	strshare_unref(email_canonical);
}

static struct command ns_canonmail = {
	.name           = "CANONMAIL",
	.desc           = N_("Displays the canonicalized form of an e-mail address."),
	.access         = PRIV_USER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &ns_cmd_canonmail,
	.help           = { .path = "nickserv/canonmail" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")
	service_named_bind_command("nickserv", &ns_canonmail);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_canonmail);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/canonmail", MODULE_UNLOAD_CAPABILITY_OK);
