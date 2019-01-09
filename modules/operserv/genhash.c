/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static void
cmd_os_genhash_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GENHASH");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: GENHASH <password>"));
		return;
	}

	const char *const result = crypt_password(parv[0]);

	if (result)
		(void) command_success_string(si, result, "%s", result);
	else
		(void) command_fail(si, fault_internalerror, _("Password hash generation failure"));

	(void) logcommand(si, CMDLOG_GET, "GENHASH");
}

static struct command cmd_os_genhash = {
	.name           = "GENHASH",
	.desc           = N_("Generates a password hash from a password."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &cmd_os_genhash_func,
	.help           = { .path = "oservice/genhash" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	(void) service_named_bind_command("operserv", &cmd_os_genhash);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &cmd_os_genhash);
}

SIMPLE_DECLARE_MODULE_V1("operserv/genhash", MODULE_UNLOAD_CAPABILITY_OK)
