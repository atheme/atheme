/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the HelpServ SERVICES command.
 *
 */

#include "atheme.h"
#include "uplink.h"

DECLARE_MODULE_V1
(
	"helpserv/services", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void helpserv_cmd_services(sourceinfo_t *si, int parc, char *parv[]);

command_t helpserv_services = { "SERVICES", N_("List all services currently running on the network."), AC_NONE, 1, helpserv_cmd_services, { .path = "helpserv/services" } };

void _modinit(module_t *m)
{
        service_named_bind_command("helpserv", &helpserv_services);
}

void _moddeinit()
{
        service_named_unbind_command("helpserv", &helpserv_services);
}

static void helpserv_cmd_services(sourceinfo_t *si, int parc, char *parv[])
{
	service_t *sptr;
	mowgli_patricia_iteration_state_t state;

	command_success_nodata(si, _("Services running on %s:"), me.netname);

	MOWGLI_PATRICIA_FOREACH(sptr, &state, services_nick)
	{
		if (sptr->conf_table == NULL)
			continue;

		command_success_nodata(si, _("%s"), sptr->nick);
	}

	command_success_nodata(si, _("More information on each service is available by messaging it like so: /msg service help"));

        return;
}
/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
