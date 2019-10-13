/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 Alexandria Wolcott <alyx@sporksmoo.net>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Channel information gatherer for statistics (Useful mostly for XMLRPC)
 */

#include <atheme.h>

static mowgli_patricia_t *ss_channel_cmds = NULL;

static void
ss_cmd_channel(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANNEL");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: CHANNEL [TOPIC|COUNT] [parameters]"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, ss_channel_cmds, "CHANNEL");
}

static void
ss_cmd_channel_topic(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANNEL TOPIC");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: CHANNEL TOPIC <#channel>"));
		return;
	}

	const struct channel *const c = channel_find(parv[0]);

	if (! c)
	{
		(void) command_fail(si, fault_nosuch_target, _("The channel \2%s\2 does not exist."), parv[0]);
		return;
	}

	if (c->modes & CMODE_SEC)
	{
		(void) command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (c->topic)
		(void) command_success_nodata(si, _("Topic for %s set by %s: %s"), c->name, c->topic_setter, c->topic);
	else
		(void) command_success_nodata(si, _("No topic set for %s"), c->name);
}

static void
ss_cmd_channel_count(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                     char ATHEME_VATTR_UNUSED **const restrict parv)
{
	const unsigned int chancount = mowgli_patricia_size(chanlist);

	(void) command_success_nodata(si, ngettext(N_("There is \2%u\2 channel on the network."),
	                                           N_("There are \2%u\2 channels on the network."),
	                                           chancount), chancount);
}

static struct command ss_channel = {
	.name           = "CHANNEL",
	.desc           = N_("Obtain various information about a channel."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &ss_cmd_channel,
	.help           = { .path = "statserv/channel" },
};

static struct command ss_channel_topic = {
	.name           = "TOPIC",
	.desc           = N_("Obtain the topic for a given channel."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ss_cmd_channel_topic,
	.help           = { .path = "" },
};

static struct command ss_channel_count = {
	.name           = "COUNT",
	.desc           = N_("Count the number of channels on the network."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ss_cmd_channel_count,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "statserv/main")

	if (! (ss_channel_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&ss_channel_topic, ss_channel_cmds);
	(void) command_add(&ss_channel_count, ss_channel_cmds);

	(void) service_named_bind_command("statserv", &ss_channel);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("statserv", &ss_channel);

	(void) mowgli_patricia_destroy(ss_channel_cmds, &command_delete_trie_cb, ss_channel_cmds);
}

SIMPLE_DECLARE_MODULE_V1("statserv/channel", MODULE_UNLOAD_CAPABILITY_OK)
