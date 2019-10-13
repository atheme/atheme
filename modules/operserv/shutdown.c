/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the OService SHUTDOWN command.
 */

#include <atheme.h>

static void
os_cmd_shutdown(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "SHUTDOWN");
	wallops("Shutting down by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_SHUTDOWN;
}

static struct command os_shutdown = {
	.name           = "SHUTDOWN",
	.desc           = N_("Shuts down services."),
	.access         = PRIV_ADMIN,
	.maxparc        = 0,
	.cmd            = &os_cmd_shutdown,
	.help           = { .path = "oservice/shutdown" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_shutdown);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_shutdown);
}

SIMPLE_DECLARE_MODULE_V1("operserv/shutdown", MODULE_UNLOAD_CAPABILITY_OK)
