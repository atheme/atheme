/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the OperServ HELP command.
 */

#include <atheme.h>

static void
os_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	if (! has_any_privs(si))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to use %s."), si->service->nick);
		return;
	}

	if (parv[0])
	{
		(void) help_display(si, si->service, parv[0], si->service->commands);
		return;
	}

	(void) help_display_prefix(si, si->service);

	(void) command_success_nodata(si, _("\2%s\2 provides essential network management services, such as\n"
	                                    "routing manipulation and access restriction. Please do not abuse\n"
	                                    "your access to \2%s\2!"), si->service->nick, si->service->nick);

	(void) help_display_newline(si);
	(void) command_help(si, si->service->commands);
	(void) help_display_moreinfo(si, si->service, NULL);
	(void) help_display_suffix(si);
}

static struct command os_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &os_cmd_help,
	.help           = { .path = "help" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	(void) service_named_bind_command("operserv", &os_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &os_help);
}

SIMPLE_DECLARE_MODULE_V1("operserv/help", MODULE_UNLOAD_CAPABILITY_OK)
