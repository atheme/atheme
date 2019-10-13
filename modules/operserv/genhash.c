/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static void
cmd_os_genhash_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! is_ircop(si->su))
	{
		(void) command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GENHASH");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: GENHASH <password> [provider]"));
		return;
	}

	const char *const password = parv[0];
	const char *const provider = parv[1];

	const struct crypt_impl *ci = crypt_get_default_provider();

	if (provider)
	{
		if (! (ci = crypt_get_named_provider(provider)))
		{
			(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "GENHASH");
			(void) command_fail(si, fault_badparams, _("The crypto provider \2%s\2 does not exist"),
			                                         provider);
			return;
		}
		if (! ci->crypt)
		{
			(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "GENHASH");
			(void) command_fail(si, fault_badparams, _("The crypto provider \2%s\2 is not capable of "
			                                           "encrypting passwords"), provider);
			return;
		}
	}
	else if (! ci)
	{
		(void) command_fail(si, fault_internalerror, _("No encryption-capable crypto provider is available"));
		return;
	}

	const char *const result = ci->crypt(password, NULL);

	if (! result)
	{
		(void) command_fail(si, fault_internalerror, _("Failed to encrypt password with crypto provider "
		                                               "\2%s\2"), ci->id);
		return;
	}

	(void) command_success_nodata(si, _("Using crypto provider \2%s\2:"), ci->id);
	(void) command_success_string(si, result, "  %s", result);
	(void) logcommand(si, CMDLOG_GET, "GENHASH (%s)", ci->id);
}

static struct command cmd_os_genhash = {
	.name           = "GENHASH",
	.desc           = N_("Generates a password hash from a password."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cmd_os_genhash_func,
	.help           = { .path = "oservice/genhash" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	(void) service_named_bind_command("operserv", &cmd_os_genhash);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &cmd_os_genhash);
}

SIMPLE_DECLARE_MODULE_V1("operserv/genhash", MODULE_UNLOAD_CAPABILITY_OK)
