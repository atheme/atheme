/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService INJECT command.
 */

#include "atheme.h"
#include "uplink.h"

static void os_cmd_inject(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_inject = { "INJECT", N_("Fakes data from the uplink (debugging tool)."), PRIV_ADMIN, 1, os_cmd_inject, { .path = "oservice/inject" } };

static void
os_cmd_inject(struct sourceinfo *si, int parc, char *parv[])
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
	parse(inject);
	injecting = false;
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
        service_named_bind_command("operserv", &os_inject);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_inject);
}

SIMPLE_DECLARE_MODULE_V1("operserv/inject", MODULE_UNLOAD_CAPABILITY_OK)
