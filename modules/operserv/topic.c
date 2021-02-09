/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains functionality which implements the OService TOPIC command.
 */

#include <atheme.h>

static void
os_cmd_topic(struct sourceinfo *si, int parc, char *parv[])
{
        char *channel = parv[0];
	char *topic = parv[1];
	struct channel *c;
	time_t prevtopicts;

        if (!channel || !topic)
        {
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOPIC");
                command_fail(si, fault_needmoreparams, _("Syntax: TOPIC <#channel> <topic>"));
                return;
        }

	if (!(c = channel_find(channel)))
	{
                command_fail(si, fault_nosuch_target, _("Channel \2%s\2 does not exist."), channel);
                return;
	}

	if (!validtopic(topic))
	{
		command_fail(si, fault_badparams, _("The new topic is invalid or too long."));
		return;
	}

	prevtopicts = c->topicts;
	topic_sts(c, si->service->me, si->service->me->nick, CURRTIME, prevtopicts, topic);
	wallops("\2%s\2 is using TOPIC on \2%s\2 (set: \2%s\2)",
		get_oper_name(si), channel, topic);
	logcommand(si, CMDLOG_ADMIN, "TOPIC: \2%s\2 on \2%s\2", topic, channel);
	command_success_nodata(si, _("Topic set to \2%s\2 on \2%s\2."), topic, channel);
}

static struct command os_topic = {
	.name           = "TOPIC",
	.desc           = N_("Changes topic on channels."),
	.access         = PRIV_OTOPIC,
	.maxparc        = 2,
	.cmd            = &os_cmd_topic,
	.help           = { .path = "oservice/topic" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

        service_named_bind_command("operserv", &os_topic);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_topic);
}

SIMPLE_DECLARE_MODULE_V1("operserv/topic", MODULE_UNLOAD_CAPABILITY_OK)
