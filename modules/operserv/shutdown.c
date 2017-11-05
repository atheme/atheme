/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SHUTDOWN command.
 */

#include "atheme.h"

SIMPLE_DECLARE_MODULE_V1("operserv/shutdown", MODULE_UNLOAD_CAPABILITY_OK,
                         _modinit, _moddeinit);

static void os_cmd_shutdown(sourceinfo_t *si, int parc, char *parv[]);

command_t os_shutdown = { "SHUTDOWN", N_("Shuts down services."), PRIV_ADMIN, 0, os_cmd_shutdown, { .path = "oservice/shutdown" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_shutdown);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_shutdown);
}

static void os_cmd_shutdown(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "SHUTDOWN");
	wallops("Shutting down by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_SHUTDOWN;
}
