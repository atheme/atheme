/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009-2010 Celestin, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains a BotServ SET command which can change
 * the BotServ settings on a channel or a bot.
 */

#include <atheme.h>

// Imported by other modules/botserv/set_*.so
extern mowgli_patricia_t *bs_set_cmdtree;
mowgli_patricia_t *bs_set_cmdtree = NULL;

static void
bs_help_set(struct sourceinfo *const restrict si, const char *const restrict subcmd)
{
	if (subcmd)
	{
		(void) help_display_as_subcmd(si, si->service, "SET", subcmd, bs_set_cmdtree);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("Help for \2SET\2:"));
	(void) help_display_newline(si);
	(void) command_success_nodata(si, _("Configures different botserv bot options."));
	(void) help_display_newline(si);
	(void) command_help(si, bs_set_cmdtree);
	(void) help_display_moreinfo(si, si->service, "SET");
	(void) help_display_suffix(si);
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
		(void) help_display_invalid(si, si->service, "SET");
		return;
	}

	parv[1] = dest;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static struct command bs_set = {
	.name           = "SET",
	.desc           = N_("Configures bot options."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &bs_cmd_set,
	.help           = { .func = &bs_help_set },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/main")

	if (! (bs_set_cmdtree = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_named_bind_command("botserv", &bs_set);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("botserv", &bs_set);

	(void) mowgli_patricia_destroy(bs_set_cmdtree, &command_delete_trie_cb, bs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("botserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
