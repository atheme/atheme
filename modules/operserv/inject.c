/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService INJECT command.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"operserv/inject", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_inject(sourceinfo_t *si, int parc, char *parv[]);

command_t os_inject = { "INJECT", N_("Fakes data from the uplink (debugging tool)."), PRIV_ADMIN, 1, os_cmd_inject, { .path = "oservice/inject" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_inject);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_inject);
}

static void os_cmd_inject(sourceinfo_t *si, int parc, char *parv[])
{
	char *inject;
	static bool injecting = false;
	inject = parv[0];

	if (!config_options.raw)
		return;

	if (!inject)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: INJECT <parameters>"));
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "INJECT: \2%s\2", inject);

	/* looks like someone INJECT'd an INJECT command.
	 * this is probably a bad thing.
	 */
	if (injecting)
	{
		command_fail(si, fault_noprivs, _("You cannot inject an INJECT command."));
		return;
	}

	injecting = true;
	irc_parse(inject);
	injecting = false;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
