/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"operserv/raw", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_raw(sourceinfo_t *si, int parc, char *parv[]);

command_t os_raw = { "RAW", N_("Sends data to the uplink."), PRIV_ADMIN, 1, os_cmd_raw, { .path = "oservice/raw" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_raw);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_raw);
}

static void os_cmd_raw(sourceinfo_t *si, int parc, char *parv[])
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
