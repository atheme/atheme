/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_help_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc)
	{
		(void) help_display(si, si->service, parv[0], si->service->commands);
		return;
	}

	(void) help_display_prefix(si, si->service);

	(void) command_success_nodata(si, _("\2%s\2 provides tools for managing groups of users and channels."),
	                              si->service->nick);

	(void) help_display_newline(si);
	(void) command_help(si, si->service->commands);
	(void) help_display_moreinfo(si, si->service, NULL);
	(void) help_display_locations(si);
	(void) help_display_suffix(si);
}

static struct command gs_cmd_help = {
	.name           = "HELP",
	.desc           = N_("Displays contextual help information."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &gs_cmd_help_func,
	.help           = { .path = "help" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_help);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/help", MODULE_UNLOAD_CAPABILITY_OK)
