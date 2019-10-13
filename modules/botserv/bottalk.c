/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Celestin, et al.
 * Copyright (C) 2016 Austin Ellis
 *
 * This file contains a BService SAY and ACT which can make
 * botserv bot talk in channel.
 */

#include <atheme.h>

static void
bs_cmd_say(struct sourceinfo *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *message = parv[1];
	char saybuf[BUFSIZE];
	struct metadata *bs;
	struct user *bot;

	if (!channel || !message)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SAY");
		command_fail(si, fault_needmoreparams, _("Syntax: SAY <#channel> <msg>"));
		return;
	}

	struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, channel);
		return;
	}

	if (!(chanacs_source_flags(mc, si) & (CA_OP | CA_AUTOOP)))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:frozen:freezer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is frozen."), mc->name);
		return;
	}

	if ((bs = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
		bot = user_find_named(bs->value);
	else
		bot = NULL;
	if (bot == NULL)
	{
		command_fail(si, fault_nosuch_key, _("\2%s\2 does not have a bot assigned."), mc->name);
		return;
	}

	if (metadata_find(mc, "private:botserv:saycaller"))
	{
		snprintf(saybuf, BUFSIZE, "%s", get_source_name(si));
		msg(bot->nick, channel, "(%s) %s", saybuf, message);
		logcommand(si, CMDLOG_DO, "SAY:\2%s\2: \2%s\2", channel, message);
	}
	else
	{

	msg(bot->nick, channel, "%s", message);
	logcommand(si, CMDLOG_DO, "SAY:\2%s\2: \2%s\2", channel, message);
	}
}

static void
bs_cmd_act(struct sourceinfo *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *message = parv[1];
	char actbuf[BUFSIZE];
	struct metadata *bs;
	struct user *bot;

	if (!channel || !message)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACT");
		command_fail(si, fault_needmoreparams, _("Syntax: ACT <#channel> <msg>"));
		return;
	}

	struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, channel);
		return;
	}

	if (!(chanacs_source_flags(mc, si) & (CA_OP | CA_AUTOOP)))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:frozen:freezer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is frozen."), mc->name);
		return;
	}

	if ((bs = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
		bot = user_find_named(bs->value);
	else
		bot = NULL;
	if (bot == NULL)
	{
		command_fail(si, fault_nosuch_key, _("\2%s\2 does not have a bot assigned."), mc->name);
		return;
	}
	if (metadata_find(mc, "private:botserv:saycaller"))
	{
		snprintf(actbuf, BUFSIZE, "%s", get_source_name(si));
		msg(bot->nick, channel, "\001ACTION (%s) %s\001", actbuf, message);
		logcommand(si, CMDLOG_DO, "ACT:\2%s\2: \2%s\2", channel, message);
	}
	else
	{

	msg(bot->nick, channel, "\001ACTION %s\001", message);
	logcommand(si, CMDLOG_DO, "ACT:\2%s\2: \2%s\2", channel, message);
	}
}

static struct command bs_say = {
	.name           = "SAY",
	.desc           = N_("Makes the bot say the given text on the given channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &bs_cmd_say,
	.help           = { .path = "botserv/say" },
};

static struct command bs_act = {
	.name           = "ACT",
	.desc           = N_("Makes the bot do the equivalent of a \"/me\" command."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &bs_cmd_act,
	.help           = { .path = "botserv/act" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "botserv/main")

	service_named_bind_command("botserv", &bs_say);
	service_named_bind_command("botserv", &bs_act);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("botserv", &bs_say);
	service_named_unbind_command("botserv", &bs_act);
}

SIMPLE_DECLARE_MODULE_V1("botserv/bottalk", MODULE_UNLOAD_CAPABILITY_OK)
