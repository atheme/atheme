/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the OService HELP command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/help", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t os_help = { "HELP", N_("Displays contextual help information."), AC_NONE, 1, os_cmd_help, { .path = "help" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_help);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_help);
}

/* HELP <command> [params] */
static void os_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!has_any_privs(si))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to use %s."), si->service->nick);
		return;
	}

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 provides essential network management services, such as\n"
					"routing manipulation and access restriction. Please do not abuse\n"
					"your access to \2%s\2!"),
				si->service->nick, si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		command_success_nodata(si, " ");

		command_help(si, si->service->commands);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, si->service->commands);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
