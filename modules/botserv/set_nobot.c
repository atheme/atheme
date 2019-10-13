/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Prevent a bot from being assigned to a channel.
 */

#include <atheme.h>

static mowgli_patricia_t **bs_set_cmdtree = NULL;

static void
bs_cmd_set_nobot(struct sourceinfo *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	struct mychan *mc;
	struct metadata *md;

	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOBOT");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> NOBOT {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
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
		metadata_add(mc, "private:botserv:no-bot", "ON");
		if ((md = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
		{
			if (mc->flags & MC_GUARD &&
					(!config_options.leave_chans ||
					 (mc->chan != NULL &&
					  MOWGLI_LIST_LENGTH(&mc->chan->members) > 1)))
				join(mc->name, chansvs.nick);
			part(mc->name, md->value);
			metadata_delete(mc, "private:botserv:bot-assigned");
			metadata_delete(mc, "private:botserv:bot-handle-fantasy");
		}
		logcommand(si, CMDLOG_SET, "SET:NOBOT:ON: \2%s\2", mc->name);
		command_success_nodata(si, _("No Bot mode is now \2ON\2 on channel %s."), mc->name);
	}
	else if(!irccasecmp(option, "OFF"))
	{
		metadata_delete(mc, "private:botserv:no-bot");
		logcommand(si, CMDLOG_SET, "SET:NOBOT:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("No Bot mode is now \2OFF\2 on channel %s."), mc->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NOBOT");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> NOBOT {ON|OFF}"));
	}
}

static struct command bs_set_nobot = {
	.name           = "NOBOT",
	.desc           = N_("Prevent a bot from being assigned to a channel."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 2,
	.cmd            = &bs_cmd_set_nobot,
	.help           = { .path = "botserv/set_nobot" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, bs_set_cmdtree, "botserv/set_core", "bs_set_cmdtree")

	command_add(&bs_set_nobot, *bs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&bs_set_nobot, *bs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("botserv/set_nobot", MODULE_UNLOAD_CAPABILITY_OK)
