/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Enable fantasy commands.
 */

#include <atheme.h>

static mowgli_patricia_t **bs_set_cmdtree = NULL;

static void
bs_cmd_set_fantasy(struct sourceinfo *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	struct mychan *mc;
	struct metadata *md;

	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET FANTASY");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> FANTASY {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!irccasecmp(option, "ON"))
	{
		if ((md = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
			metadata_add(mc, "private:botserv:bot-handle-fantasy", md->value);

		logcommand(si, CMDLOG_SET, "SET:FANTASY:ON: \2%s\2", mc->name);

		command_success_nodata(si, _("Fantasy mode is now \2ON\2 on channel %s."), mc->name);
	}
	else if(!irccasecmp(option, "OFF"))
	{
		metadata_delete(mc, "private:botserv:bot-handle-fantasy");
		logcommand(si, CMDLOG_SET, "SET:FANTASY:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("Fantasy mode is now \2OFF\2 on channel %s."), mc->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET FANTASY");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> FANTASY {ON|OFF}"));
	}
}

static struct command bs_set_fantasy = {
	.name           = "FANTASY",
	.desc           = N_("Enable fantasy commands."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &bs_cmd_set_fantasy,
	.help           = { .path = "botserv/set_fantasy" },
};

static void
bs_set_fantasy_config_ready(void *unused)
{
	if (chansvs.fantasy)
		bs_set_fantasy.access = AC_NONE;
	else
		bs_set_fantasy.access = AC_DISABLED;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, bs_set_cmdtree, "botserv/set_core", "bs_set_cmdtree")

	command_add(&bs_set_fantasy, *bs_set_cmdtree);

	hook_add_config_ready(bs_set_fantasy_config_ready);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&bs_set_fantasy, *bs_set_cmdtree);

	hook_del_config_ready(bs_set_fantasy_config_ready);
}

SIMPLE_DECLARE_MODULE_V1("botserv/set_fantasy", MODULE_UNLOAD_CAPABILITY_OK)
