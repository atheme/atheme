/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService VOICE functions.
 */

#include "atheme.h"
#include "chanserv.h"

static void cs_cmd_voice(struct sourceinfo *si, int parc, char *parv[]);
static void cs_cmd_devoice(struct sourceinfo *si, int parc, char *parv[]);

struct command cs_voice = { "VOICE", N_("Gives channel voice to a user."),
                         AC_NONE, 2, cs_cmd_voice, { .path = "cservice/op_voice" } };
struct command cs_devoice = { "DEVOICE", N_("Removes channel voice from a user."),
                         AC_NONE, 2, cs_cmd_devoice, { .path = "cservice/op_voice" } };

static void
mod_init(struct module *const restrict m)
{
        service_named_bind_command("chanserv", &cs_voice);
        service_named_bind_command("chanserv", &cs_devoice);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_voice);
	service_named_unbind_command("chanserv", &cs_devoice);
}

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
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	nicks = (!nick ? sstrdup(si->su->nick) : sstrdup(nick));
	prefix_action_set_all(&voice_actions, voicing, nicks);
	free(nicks);

	MOWGLI_LIST_FOREACH(n, voice_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		voice = act->en;

		/* figure out who we're going to voice */
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (!chanacs_source_has_flag(mc, si, CA_VOICE) && (tu != si->su || !chanacs_source_has_flag(mc, si, CA_AUTOVOICE)))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to (de)voice \2%s\2 on \2%s\2."), nick, mc->name);
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

		modestack_mode_param(chansvs.nick, mc->chan, voice ? MTYPE_ADD : MTYPE_DEL, 'v', CLIENT_NAME(tu));
		if (voice)
			cu->modes |= CSTATUS_VOICE;
		else
			cu->modes &= ~CSTATUS_VOICE;

		if (si->c == NULL && tu != si->su)
			change_notify(chansvs.nick, tu, "You have been %svoiced on %s by %s", voice ? "" : "de", mc->name, get_source_name(si));

		logcommand(si, CMDLOG_DO, "%sVOICE: \2%s!%s@%s\2 on \2%s\2", voice ? "": "DE", tu->nick, tu->user, tu->vhost, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("\2%s\2 has been %svoiced on \2%s\2."), tu->nick, voice ? "" : "de", mc->name);
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

SIMPLE_DECLARE_MODULE_V1("chanserv/voice", MODULE_UNLOAD_CAPABILITY_OK)
