/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2011 William Pitcock, et al.
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the OperServ SET command.
 */

#include <atheme.h>

// Imported by other modules/operserv/set_* & modules/proxyscan/dnsbl
extern mowgli_patricia_t *os_set_cmdtree;
mowgli_patricia_t *os_set_cmdtree = NULL;

static void
os_help_set_func(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, si->service, "SET", subcmd, os_set_cmdtree);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("Help for \2SET\2:"));
	(void) help_display_newline(si);

	(void) command_success_nodata(si, _("SET allows you to set various control flags for\n"
	                                    "services that changes the way certain operations\n"
	                                    "are performed by them. Note that all settings\n"
	                                    "will be re-set to the values in the configuration\n"
	                                    "file when services is rehashed or restarted."));

	(void) help_display_newline(si);
	(void) command_help(si, os_set_cmdtree);
	(void) help_display_moreinfo(si, si->service, "SET");
	(void) help_display_suffix(si);
}

static void
os_cmd_set_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET <setting> <parameters>"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_set_cmdtree, "SET");
}

static struct command os_cmd_set = {
	.name           = "SET",
	.desc           = N_("Sets various control flags."),
	.access         = PRIV_ADMIN,
	.maxparc        = 2,
	.cmd            = &os_cmd_set_func,
	.help           = { .func = &os_help_set_func },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (os_set_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_named_bind_command("operserv", &os_cmd_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &os_cmd_set);

	(void) mowgli_patricia_destroy(os_set_cmdtree, &command_delete_trie_cb, os_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("operserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
