/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 */

#include "atheme.h"

DECLARE_MODULE_V1("operserv/restart", MODULE_UNLOAD_CAPABILITY_OK,
                  _modinit, _moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);

static void os_cmd_restart(sourceinfo_t *si, int parc, char *parv[]);

command_t os_restart = { "RESTART", N_("Restart services."), PRIV_ADMIN, 0, os_cmd_restart, { .path = "oservice/restart" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_restart);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_restart);
}

static void os_cmd_restart(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "RESTART");
	wallops("Restarting by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_RESTART;
}
