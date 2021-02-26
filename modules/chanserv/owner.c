/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2008 William Pitcock, et al.
 *
 * This file contains code for the CService OWNER functions.
 */

#include <atheme.h>
#include "chanserv.h"

static mowgli_list_t owner_actions;

static void
cmd_owner(struct sourceinfo *si, bool ownering, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	struct mychan *mc;
	struct user *tu;
	struct chanuser *cu;
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
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_USEOWNER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
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
	prefix_action_set_all(&owner_actions, ownering, nicks);
	sfree(nicks);

	MOWGLI_LIST_FOREACH(n, owner_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		owner = act->en;

		// figure out who we're going to op
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		// SECURE check; we can skip this if deownering or sender == target, because we already verified
		if (owner && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			command_fail(si, fault_noprivs, _("\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access."), mc->name, tu->nick);
			continue;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
			continue;
		}

		if (owner)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->owner_mchar[1], CLIENT_NAME(tu));
			cu->modes |= CSTATUS_OWNER;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had owner (+%c) status given to you on "
				                                "\2%s\2 by \2%s\2", ircd->owner_mchar[1], mc->name,
				                                get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had owner (+%c) status given to them on "
				                             "\2%s\2"), tu->nick, ircd->owner_mchar[1], mc->name);

			logcommand(si, CMDLOG_DO, "OWNER: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                           mc->name);
		}
		else
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->owner_mchar[1], CLIENT_NAME(tu));
			cu->modes &= ~CSTATUS_OWNER;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had owner (+%c) status taken from you on "
				                                "\2%s\2 by \2%s\2", ircd->owner_mchar[1], mc->name,
				                                get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had owner (+%c) status taken from them on "
				                             "\2%s\2"), tu->nick, ircd->owner_mchar[1], mc->name);

			logcommand(si, CMDLOG_DO, "OWNER: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                           mc->name);
		}
	}

	prefix_action_clear(&owner_actions);
}

static void
cs_cmd_owner(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OWNER");
		command_fail(si, fault_needmoreparams, _("Syntax: OWNER <#channel> [nickname] [...]"));
		return;
	}

	cmd_owner(si, true, parc, parv);
}

static void
cs_cmd_deowner(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOWNER");
		command_fail(si, fault_needmoreparams, _("Syntax: DEOWNER <#channel> [nickname] [...]"));
		return;
	}

	cmd_owner(si, false, parc, parv);
}

static struct command cs_owner = {
	.name           = "OWNER",
	.desc           = N_("Gives the channel owner flag to a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_owner,
	.help           = { .path = "cservice/owner" },
};

static struct command cs_deowner = {
	.name           = "DEOWNER",
	.desc           = N_("Removes channel owner flag from a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_deowner,
	.help           = { .path = "cservice/owner" },
};

static void
mod_init(struct module *const restrict m)
{
	if (ircd != NULL && !ircd->uses_owner)
	{
		slog(LG_INFO, "Module %s requires owner support, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_owner);
        service_named_bind_command("chanserv", &cs_deowner);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_owner);
	service_named_unbind_command("chanserv", &cs_deowner);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/owner", MODULE_UNLOAD_CAPABILITY_OK)
