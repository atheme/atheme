/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple module inspector.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/modinspect", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_modinspect(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modinspect = { "MODINSPECT", N_("Displays information about loaded modules."), PRIV_SERVER_AUSPEX, 1, os_cmd_modinspect, { .path = "oservice/modinspect" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_modinspect);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_modinspect);
}

static void os_cmd_modinspect(sourceinfo_t *si, int parc, char *parv[])
{
	char *mname = parv[0];
	module_t *m;

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

	/* Is there a header? */
	if (!m->header)
	{
		command_fail(si, fault_unimplemented, _("\2%s\2 cannot be inspected."), mname);
		return;
	}

	command_success_nodata(si, _("Information on \2%s\2:"), mname);
	command_success_nodata(si, _("Name       : %s"), m->header->name);
	command_success_nodata(si, _("Address    : %p"), m->address);
	command_success_nodata(si, _("Entry point: %p"), m->header->modinit);
	command_success_nodata(si, _("Exit point : %p"), m->header->deinit);
	command_success_nodata(si, _("SDK Serial : %s"), m->header->serial);
	command_success_nodata(si, _("Version    : %s"), m->header->version);
	command_success_nodata(si, _("Vendor     : %s"), m->header->vendor);
	command_success_nodata(si, _("Permanent  : %s"), m->header->norestart ? "Yes" : "No");
	command_success_nodata(si, _("*** \2End of Info\2 ***"));
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
