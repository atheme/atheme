/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OWNER functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"

DECLARE_MODULE_V1
(
	"chanserv/owner", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_owner(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deowner(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_owner = { "OWNER", N_("Gives the channel owner flag to a user."),
                        AC_NONE, 2, cs_cmd_owner, { .path = "cservice/owner" } };
command_t cs_deowner = { "DEOWNER", N_("Removes channel owner flag from a user."),
                        AC_NONE, 2, cs_cmd_deowner, { .path = "cservice/owner" } };

void _modinit(module_t *m)
{
	if (ircd != NULL && !ircd->uses_owner)
	{
		slog(LG_INFO, "Module %s requires owner support, refusing to load.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

        service_named_bind_command("chanserv", &cs_owner);
        service_named_bind_command("chanserv", &cs_deowner);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_owner);
	service_named_unbind_command("chanserv", &cs_deowner);
}

static mowgli_list_t owner_actions;

static void cmd_owner(sourceinfo_t *si, bool ownering, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char *nicks;
	bool owner;
	mowgli_node_t *n;

	if (ircd->uses_owner == false)
	{
		command_fail(si, fault_noprivs, _("The IRCd software you are running does not support this feature."));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_USEOWNER))
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
	prefix_action_set_all(&owner_actions, ownering, nicks);
	free(nicks);

	MOWGLI_LIST_FOREACH(n, owner_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		owner = act->en;

		/* figure out who we're going to op */
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		/* SECURE check; we can skip this if deownering or sender == target, because we already verified */
		if (owner && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			command_fail(si, fault_noprivs, _("\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access."), mc->name, tu->nick);
			continue;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, mc->chan, owner ? MTYPE_ADD : MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(tu));
		if (owner)
			cu->modes |= CSTATUS_OWNER;
		else
			cu->modes &= ~CSTATUS_OWNER;

		if (si->c == NULL && tu != si->su)
			change_notify(chansvs.nick, tu, "You have been %sset as owner on %s by %s", owner ? "" : "un", mc->name, get_source_name(si));

		logcommand(si, CMDLOG_DO, "%sOWNER: \2%s!%s@%s\2 on \2%s\2", owner ? "" : "DE", tu->nick, tu->user, tu->vhost, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _("\2%s\2 has been %sset as owner on \2%s\2."), tu->nick, owner ? "" : "un", mc->name);
	}

	prefix_action_clear(&owner_actions);
}

static void cs_cmd_owner(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OWNER");
		command_fail(si, fault_needmoreparams, _("Syntax: OWNER <#channel> [nickname] [...]"));
		return;
	}

	cmd_owner(si, true, parc, parv);
}

static void cs_cmd_deowner(sourceinfo_t *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOWNER");
		command_fail(si, fault_needmoreparams, _("Syntax: DEOWNER <#channel> [nickname] [...]"));
		return;
	}

	cmd_owner(si, false, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
