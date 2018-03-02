/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 */

#include "atheme.h"

// Imported by other modules/nickserv/set_*.so
mowgli_patricia_t *ns_set_cmdtree;

// HELP SET
static void
ns_help_set(struct sourceinfo *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), nicksvs.nick);
		command_success_nodata(si, _("Help for \2SET\2:"));
		command_success_nodata(si, " ");
		if (nicksvs.no_nick_ownership)
			command_success_nodata(si, _("SET allows you to set various control flags\n"
						"for accounts that change the way certain\n"
						"operations are performed on them."));
		else
			command_success_nodata(si, _("SET allows you to set various control flags\n"
						"for nicknames that change the way certain\n"
						"operations are performed on them."));
		command_success_nodata(si, " ");
		command_help(si, ns_set_cmdtree);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information, use \2/msg %s HELP SET \37command\37\2."), nicksvs.nick);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display_as_subcmd(si, si->service, "SET", subcmd, ns_set_cmdtree);
}

// SET <setting> <parameters>
static void
ns_cmd_set(struct sourceinfo *si, int parc, char *parv[])
{
	char *setting = parv[0];
	struct command *c;

	if (setting == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <setting> <parameters>"));
		return;
	}

	// take the command through the hash table
        if ((c = command_find(ns_set_cmdtree, setting)))
	{
		command_exec(si->service, si, c, parc - 1, parv + 1);
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid set command. Use \2/%s%s HELP SET\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", nicksvs.nick);
	}
}

static struct command ns_set = { "SET", N_("Sets various control flags."), AC_AUTHENTICATED, 2, ns_cmd_set, { .func = ns_help_set } };

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("nickserv", &ns_set);

	ns_set_cmdtree = mowgli_patricia_create(strcasecanon);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_set);
	mowgli_patricia_destroy(ns_set_cmdtree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
