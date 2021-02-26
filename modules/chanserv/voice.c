/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService VOICE functions.
 */

#include <atheme.h>
#include "chanserv.h"

static mowgli_list_t voice_actions;

static void
cmd_voice(struct sourceinfo *si, bool voicing, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	char *nicks;
	bool voice;
	struct mychan *mc;
	struct user *tu;
	struct chanuser *cu;
	mowgli_node_t *n;

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, chan);
		return;
	}

	nicks = (!nick ? sstrdup(si->su->nick) : sstrdup(nick));
	prefix_action_set_all(&voice_actions, voicing, nicks);
	sfree(nicks);

	MOWGLI_LIST_FOREACH(n, voice_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		voice = act->en;

		// figure out who we're going to voice
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (!chanacs_source_has_flag(mc, si, CA_VOICE) && (tu != si->su || !chanacs_source_has_flag(mc, si, CA_AUTOVOICE)))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
			continue;
		}

		if (voice)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, 'v', CLIENT_NAME(tu));
			cu->modes |= CSTATUS_VOICE;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had voice (+v) status given to you on "
				                                "\2%s\2 by \2%s\2", mc->name, get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had voice (+v) status given to them on "
				                             "\2%s\2"), tu->nick, mc->name);

			logcommand(si, CMDLOG_DO, "VOICE: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                           mc->name);
		}
		else
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, 'v', CLIENT_NAME(tu));
			cu->modes &= ~CSTATUS_VOICE;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had voice (+v) status taken from you on "
				                                "\2%s\2 by \2%s\2", mc->name, get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had voice (+v) status taken from them on "
				                             "\2%s\2"), tu->nick, mc->name);

			logcommand(si, CMDLOG_DO, "DEVOICE: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                             mc->name);
		}
	}

	prefix_action_clear(&voice_actions);
}

static void
cs_cmd_voice(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VOICE");
		command_fail(si, fault_needmoreparams, _("Syntax: VOICE <#channel> [nickname] [...]"));
		return;
	}

	cmd_voice(si, true, parc, parv);
}

static void
cs_cmd_devoice(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEVOICE");
		command_fail(si, fault_needmoreparams, _("Syntax: DEVOICE <#channel> [nickname] [...]"));
		return;
	}

	cmd_voice(si, false, parc, parv);
}

static struct command cs_voice = {
	.name           = "VOICE",
	.desc           = N_("Gives channel voice to a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_voice,
	.help           = { .path = "cservice/op_voice" },
};

static struct command cs_devoice = {
	.name           = "DEVOICE",
	.desc           = N_("Removes channel voice from a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_devoice,
	.help           = { .path = "cservice/op_voice" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_voice);
        service_named_bind_command("chanserv", &cs_devoice);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_voice);
	service_named_unbind_command("chanserv", &cs_devoice);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/voice", MODULE_UNLOAD_CAPABILITY_OK)
