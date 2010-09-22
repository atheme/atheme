/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService SHUTDOWN command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/shutdown", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_shutdown(sourceinfo_t *si, int parc, char *parv[]);

command_t os_shutdown = { "SHUTDOWN", N_("Shuts down services."), PRIV_ADMIN, 0, os_cmd_shutdown, { .path = "oservice/shutdown" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_shutdown);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_shutdown);
}

static void os_cmd_shutdown(sourceinfo_t *si, int parc, char *parv[])
{
	logcommand(si, CMDLOG_ADMIN, "SHUTDOWN");
	wallops("Shutting down by request of \2%s\2.", get_oper_name(si));

	runflags |= RF_SHUTDOWN;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
