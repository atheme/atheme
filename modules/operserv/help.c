/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the OService HELP command.
 */

#include "atheme.h"

static void os_cmd_help(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_help = { "HELP", N_("Displays contextual help information."), AC_NONE, 1, os_cmd_help, { .path = "help" } };

/* HELP <command> [params] */
static void
os_cmd_help(struct sourceinfo *si, int parc, char *parv[])
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

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("operserv", &os_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_help);
}

SIMPLE_DECLARE_MODULE_V1("operserv/help", MODULE_UNLOAD_CAPABILITY_OK)
