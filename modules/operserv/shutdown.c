/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SHUTDOWN command.
 */

#include "atheme.h"

static void os_cmd_shutdown(struct sourceinfo *si, int parc, char *parv[]);

struct command os_shutdown = { "SHUTDOWN", N_("Shuts down services."), PRIV_ADMIN, 0, os_cmd_shutdown, { .path = "oservice/shutdown" } };

static void
mod_init(struct module *const restrict m)
{
        service_named_bind_command("operserv", &os_shutdown);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_shutdown);
}

static void
os_cmd_shutdown(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "SHUTDOWN");
	wallops("Shutting down by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_SHUTDOWN;
}

SIMPLE_DECLARE_MODULE_V1("operserv/shutdown", MODULE_UNLOAD_CAPABILITY_OK)
