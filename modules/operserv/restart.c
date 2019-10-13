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
os_cmd_restart(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "RESTART");
	wallops("Restarting by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_RESTART;
}

static struct command os_restart = {
	.name           = "RESTART",
	.desc           = N_("Restart services."),
	.access         = PRIV_ADMIN,
	.maxparc        = 0,
	.cmd            = &os_cmd_restart,
	.help           = { .path = "oservice/restart" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_restart);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_restart);
}

SIMPLE_DECLARE_MODULE_V1("operserv/restart", MODULE_UNLOAD_CAPABILITY_OK)
