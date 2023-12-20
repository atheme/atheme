/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 William Pitcock, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the CService SET command.
 */

#include <atheme.h>

// Imported by other modules/nickserv/set_*.so
extern mowgli_patricia_t *ns_set_cmdtree;
mowgli_patricia_t *ns_set_cmdtree = NULL;

static void
ns_help_set(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, si->service, "SET", subcmd, ns_set_cmdtree);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("Help for \2SET\2:"));
	(void) help_display_newline(si);

	if (nicksvs.no_nick_ownership)
		(void) command_success_nodata(si, _("SET allows you to set various control flags for\n"
		                                    "accounts that change the way certain operations\n"
		                                    "are performed on them."));
	else
		(void) command_success_nodata(si, _("SET allows you to set various control flags for\n"
		                                    "nicknames that change the way certain operations\n"
		                                    "are performed on them."));

	(void) help_display_newline(si);
	(void) command_help(si, ns_set_cmdtree);
	(void) help_display_moreinfo(si, si->service, "SET");
	(void) help_display_suffix(si);
}

static void
ns_cmd_set(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET <setting> <parameters>"));
		return;
	}

	(void) subcommand_dispatch_simple(nicksvs.me, si, parc, parv, ns_set_cmdtree, "SET");
}

static struct command ns_set = {
	.name           = "SET",
	.desc           = N_("Sets various account parameters."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &ns_cmd_set,
	.help           = { .func = &ns_help_set },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	if (! (ns_set_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_named_bind_command("nickserv", &ns_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("nickserv", &ns_set);

	(void) mowgli_patricia_destroy(ns_set_cmdtree, &command_delete_trie_cb, ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
