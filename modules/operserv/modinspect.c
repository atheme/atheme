/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * A simple module inspector.
 */

#include "atheme.h"

static void
os_cmd_modinspect(struct sourceinfo *si, int parc, char *parv[])
{
	char *mname = parv[0];
	struct module *m;

	if (!mname)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODINSPECT");
		command_fail(si, fault_needmoreparams, _("Syntax: MODINSPECT <module>"));
		return;
	}

	logcommand(si, CMDLOG_GET, "MODINSPECT: \2%s\2", mname);

	m = module_find_published(mname);

	if (!m)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not loaded."), mname);
		return;
	}

	// Is there a header?
	if (!m->header)
	{
		command_fail(si, fault_unimplemented, _("\2%s\2 cannot be inspected."), mname);
		return;
	}

	command_success_nodata(si, _("Information on \2%s\2:"), mname);
	command_success_nodata(si, _("Name       : %s"), m->name);
	command_success_nodata(si, _("Address    : %p"), m->address);
	command_success_nodata(si, _("Entry point: %p"), m->header->modinit);
	command_success_nodata(si, _("Exit point : %p"), m->header->deinit);
	command_success_nodata(si, _("SDK Serial : %s"), m->header->serial);
	command_success_nodata(si, _("Version    : %s"), m->header->version);
	command_success_nodata(si, _("Vendor     : %s"), m->header->vendor);
	command_success_nodata(si, _("Can unload : %s"), m->can_unload == MODULE_UNLOAD_CAPABILITY_OK ? "Yes" :
					( m->can_unload == MODULE_UNLOAD_CAPABILITY_NEVER ? "No" : "Reload only"));
	command_success_nodata(si, _("*** \2End of Info\2 ***"));
}

static struct command os_modinspect = {
	.name           = "MODINSPECT",
	.desc           = N_("Displays information about loaded modules."),
	.access         = PRIV_SERVER_AUSPEX,
	.maxparc        = 1,
	.cmd            = &os_cmd_modinspect,
	.help           = { .path = "oservice/modinspect" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("operserv", &os_modinspect);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_modinspect);
}

SIMPLE_DECLARE_MODULE_V1("operserv/modinspect", MODULE_UNLOAD_CAPABILITY_OK)
