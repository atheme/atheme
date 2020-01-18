/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Prevent a bot from being assigned by non IRC operators.
 */

#include <atheme.h>

static struct botserv_main_symbols *bs_main_syms = NULL;
static mowgli_patricia_t **bs_set_cmdtree = NULL;

static void
bs_cmd_set_private(struct sourceinfo *si, int parc, char *parv[])
{
	char *botserv = parv[0];
	char *option = parv[1];
	struct botserv_bot *bot;

	if (parc < 2 || !botserv || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PRIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <botnick> PRIVATE {ON|OFF}"));
		return;
	}

	bot = bs_main_syms->bot_find(botserv);
	if (!bot)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a bot."), botserv);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
		return;
	}

	if (!has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!irccasecmp(option, "ON"))
	{
		bot->private = true;
		logcommand(si, CMDLOG_SET, "SET:PRIVATE:ON: \2%s\2", bot->nick);
		command_success_nodata(si, _("Private mode of bot \2%s\2 is now \2ON\2."), bot->nick);
	}
	else if(!irccasecmp(option, "OFF"))
	{
		bot->private = false;
		logcommand(si, CMDLOG_SET, "SET:PRIVATE:OFF: \2%s\2", bot->nick);
		command_success_nodata(si, _("Private mode of bot \2%s\2 is now \2OFF\2."), bot->nick);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PRIVATE");
		command_fail(si, fault_badparams, _("Syntax: SET <botnick> PRIVATE {ON|OFF}"));
	}
}

static struct command bs_set_private = {
	.name           = "PRIVATE",
	.desc           = N_("Prevent a bot from being assigned by non IRC operators."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 2,
	.cmd            = &bs_cmd_set_private,
	.help           = { .path = "botserv/set_private" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, bs_main_syms, "botserv/main", "botserv_main_symbols")
	MODULE_TRY_REQUEST_SYMBOL(m, bs_set_cmdtree, "botserv/set_core", "bs_set_cmdtree")

	command_add(&bs_set_private, *bs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&bs_set_private, *bs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("botserv/set_private", MODULE_UNLOAD_CAPABILITY_OK)
