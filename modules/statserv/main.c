/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 Alexandria Wolcott <alyx@sporksmoo.net>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the body of StatServ.
 */

#include <atheme.h>

static struct service *statsvs = NULL;

static void
ss_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	if (parv[0])
	{
		(void) help_display(si, si->service, parv[0], si->service->commands);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("\2%s\2 records various network statistics."), si->service->nick);
	(void) help_display_newline(si);
	(void) command_help(si, si->service->commands);
	(void) help_display_moreinfo(si, si->service, NULL);
	(void) help_display_suffix(si);
}

static struct command ss_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ss_cmd_help,
	.help           = { .path = "help" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! (statsvs = service_add("statserv", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_bind_command(statsvs, &ss_help);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_delete(statsvs);
}

SIMPLE_DECLARE_MODULE_V1("statserv/main", MODULE_UNLOAD_CAPABILITY_OK)
