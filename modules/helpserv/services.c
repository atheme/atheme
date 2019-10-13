/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains functionality which implements the HelpServ SERVICES command.
 */

#include <atheme.h>

static void
helpserv_cmd_services(struct sourceinfo *si, int parc, char *parv[])
{
	struct service *sptr;
	mowgli_patricia_iteration_state_t state;

	command_success_nodata(si, _("Services running on %s:"), me.netname);

	MOWGLI_PATRICIA_FOREACH(sptr, &state, services_nick)
	{
		if (!sptr->botonly)
			command_success_nodata(si, "%s", sptr->nick);
	}

	command_success_nodata(si, _("More information on each service is available by messaging it like so: "
	                             "\2/msg <service> HELP\2"));
}

static struct command helpserv_services = {
	.name           = "SERVICES",
	.desc           = N_("List all services currently running on the network."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &helpserv_cmd_services,
	.help           = { .path = "helpserv/services" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "helpserv/main")

	service_named_bind_command("helpserv", &helpserv_services);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("helpserv", &helpserv_services);
}

SIMPLE_DECLARE_MODULE_V1("helpserv/services", MODULE_UNLOAD_CAPABILITY_OK)
