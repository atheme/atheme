/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a BService SET which can change
 * botserv settings on channel or bot.
 */

#include "atheme.h"
#include "botserv.h"

// Imported by other modules/botserv/set_*.so
mowgli_patricia_t *bs_set_cmdtree = NULL;

static void bs_help_set(struct sourceinfo *si, const char *subcmd);
static void bs_cmd_set(struct sourceinfo *si, int parc, char *parv[]);

static struct command bs_set = { "SET", N_("Configures bot options."), AC_NONE, 3, bs_cmd_set, { .func =  bs_help_set } };

/* ******************************************************************** */

static void
bs_help_set(struct sourceinfo *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->disp);
		command_success_nodata(si, _("Help for \2SET\2:"));
		command_success_nodata(si, " ");
		command_success_nodata(si, _("Configures different botserv bot options."));
		command_success_nodata(si, " ");
		command_help(si, bs_set_cmdtree);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more specific help use \2/msg %s HELP SET \37command\37\2."), si->service->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display_as_subcmd(si, si->service, "SET", subcmd, bs_set_cmdtree);
}

static void
bs_cmd_set(struct sourceinfo *si, int parc, char *parv[])
{
	char *dest;
	char *cmd;
	struct command *c;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <destination> <setting> <parameters>"));
		return;
	}

	dest = parv[0];
	cmd = parv[1];
	c = command_find(bs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = dest;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("botserv", &bs_set);

	bs_set_cmdtree = mowgli_patricia_create(strcasecanon);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("botserv", &bs_set);

	mowgli_patricia_destroy(bs_set_cmdtree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("botserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
