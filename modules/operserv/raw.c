/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the OService RAW command.
 */

#include <atheme.h>

static void
os_cmd_raw(struct sourceinfo *si, int parc, char *parv[])
{
	char *s = parv[0];

	if (!config_options.raw)
		return;

	if (!s)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RAW");
		command_fail(si, fault_needmoreparams, _("Syntax: RAW <parameters>"));
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "RAW: \2%s\2", s);
	sts("%s", s);
}

static struct command os_raw = {
	.name           = "RAW",
	.desc           = N_("Sends data to the uplink."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_raw,
	.help           = { .path = "oservice/raw" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_raw);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_raw);
}

SIMPLE_DECLARE_MODULE_V1("operserv/raw", MODULE_UNLOAD_CAPABILITY_OK)
