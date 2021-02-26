/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService OP functions.
 */

#include <atheme.h>
#include "chanserv.h"

static mowgli_list_t halfop_actions;

static void
cmd_halfop(struct sourceinfo *si, bool halfopping, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	struct mychan *mc;
	struct user *tu;
	struct chanuser *cu;
	char *nicks;
	bool halfop;
	mowgli_node_t *n;

	if (!ircd->uses_halfops)
	{
		command_fail(si, fault_noprivs, _("Your IRC server does not support halfops."));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_HALFOP))
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
	prefix_action_set_all(&halfop_actions, halfopping, nicks);
	sfree(nicks);

	MOWGLI_LIST_FOREACH(n, halfop_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		halfop = act->en;

		// figure out who we're going to halfop
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		// SECURE check; we can skip this if deopping or sender == target, because we already verified
		if (halfop && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP) && !chanacs_user_has_flag(mc, tu, CA_AUTOHALFOP))
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

		if (halfop)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->halfops_mchar[1], CLIENT_NAME(tu));
			cu->modes |= ircd->halfops_mode;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had halfop (+%c) status given to you on "
				                                "\2%s\2 by \2%s\2", ircd->halfops_mchar[1], mc->name,
				                                get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had halfop (+%c) status given to them on "
				                             "\2%s\2"), tu->nick, ircd->halfops_mchar[1], mc->name);

			logcommand(si, CMDLOG_DO, "HALFOP: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                            mc->name);
		}
		else
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->halfops_mchar[1], CLIENT_NAME(tu));
			cu->modes &= ~ircd->halfops_mode;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had halfop (+%c) status taken from you on "
				                                "\2%s\2 by \2%s\2", ircd->halfops_mchar[1], mc->name,
				                                get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had halfop (+%c) status taken from them on "
				                             "\2%s\2"), tu->nick, ircd->halfops_mchar[1], mc->name);

			logcommand(si, CMDLOG_DO, "DEHALFOP: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                              mc->name);
		}
	}

	prefix_action_clear(&halfop_actions);
}

static void
cs_cmd_halfop(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HALFOP");
		command_fail(si, fault_needmoreparams, _("Syntax: HALFOP <#channel> [nickname] [...]"));
		return;
	}

	cmd_halfop(si, true, parc, parv);
}

static void
cs_cmd_dehalfop(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEHALFOP");
		command_fail(si, fault_needmoreparams, _("Syntax: DEHALFOP <#channel> [nickname] [...]"));
		return;
	}

	cmd_halfop(si, false, parc, parv);
}

static struct command cs_halfop = {
	.name           = "HALFOP",
	.desc           = N_("Gives channel halfops to a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_halfop,
	.help           = { .path = "cservice/halfop" },
};

static struct command cs_dehalfop = {
	.name           = "DEHALFOP",
	.desc           = N_("Removes channel halfops from a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_dehalfop,
	.help           = { .path = "cservice/halfop" },
};

static void
mod_init(struct module *const restrict m)
{
	if (ircd != NULL && !ircd->uses_halfops)
	{
		slog(LG_INFO, "Module %s requires halfop support, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_halfop);
        service_named_bind_command("chanserv", &cs_dehalfop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_halfop);
	service_named_unbind_command("chanserv", &cs_dehalfop);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/halfop", MODULE_UNLOAD_CAPABILITY_OK)
