/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService VOICE functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"

DECLARE_MODULE_V1
(
	"chanserv/voice", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_voice(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_devoice(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_voice = { "VOICE", N_("Gives channel voice to a user."),
                         AC_NONE, 2, cs_cmd_voice, { .path = "cservice/op_voice" } };
command_t cs_devoice = { "DEVOICE", N_("Removes channel voice from a user."),
                         AC_NONE, 2, cs_cmd_devoice, { .path = "cservice/op_voice" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_voice);
        service_named_bind_command("chanserv", &cs_devoice);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_voice);
	service_named_unbind_command("chanserv", &cs_devoice);
}

static mowgli_list_t voice_actions;

static void cmd_voice(sourceinfo_t *si, bool voicing, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	char *nicks;
	bool voice;
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	mowgli_node_t *n;

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_VOICE) && (tu != si->su ||
				!chanacs_source_has_flag(mc, si, CA_AUTOVOICE)))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	nicks = (!nick ? strdup(si->su->nick) : strdup(nick));
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

static void cs_cmd_voice(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "VOICE");
		command_fail(si, fault_needmoreparams, _("Syntax: VOICE <#channel> [nickname] [...]"));
		return;
	}

	cmd_voice(si, true, parc, parv);
}

static void cs_cmd_devoice(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEVOICE");
		command_fail(si, fault_needmoreparams, _("Syntax: DEVOICE <#channel> [nickname] [...]"));
		return;
	}

	cmd_voice(si, false, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
