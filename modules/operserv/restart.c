/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 */

#include "atheme.h"

static void os_cmd_restart(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_restart = { "RESTART", N_("Restart services."), PRIV_ADMIN, 0, os_cmd_restart, { .path = "oservice/restart" } };

static void
mod_init(struct module *const restrict m)
{
        service_named_bind_command("operserv", &os_restart);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_restart);
}

static void
os_cmd_restart(struct sourceinfo *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "RESTART");
	wallops("Restarting by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_RESTART;
}

SIMPLE_DECLARE_MODULE_V1("operserv/restart", MODULE_UNLOAD_CAPABILITY_OK)
