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

static mowgli_list_t protect_actions;

static void
cmd_protect(struct sourceinfo *si, bool protecting, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	struct mychan *mc;
	struct user *tu;
	struct chanuser *cu;
	char *nicks;
	bool protect;
	mowgli_node_t *n;

	if (ircd->uses_protect == false)
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

	if (!chanacs_source_has_flag(mc, si, CA_USEPROTECT))
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
	prefix_action_set_all(&protect_actions, protecting, nicks);
	sfree(nicks);

	MOWGLI_LIST_FOREACH(n, protect_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		protect = act->en;

		// figure out who we're going to protect
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (is_internal_client(tu))
			continue;

		// SECURE check; we can skip this if deprotecting or sender == target, because we already verified
		if (protect && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
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

		if (protect)
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_ADD, ircd->protect_mchar[1], CLIENT_NAME(tu));
			cu->modes |= CSTATUS_PROTECT;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had protected (+%c) status given to you on "
				                                "\2%s\2 by \2%s\2", ircd->protect_mchar[1], mc->name,
				                                get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had protected (+%c) status given to them on "
				                             "\2%s\2"), tu->nick, ircd->protect_mchar[1], mc->name);

			logcommand(si, CMDLOG_DO, "PROTECT: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                             mc->name);
		}
		else
		{
			modestack_mode_param(chansvs.nick, mc->chan, MTYPE_DEL, ircd->protect_mchar[1], CLIENT_NAME(tu));
			cu->modes &= ~CSTATUS_PROTECT;

			if (! si->c && tu != si->su)
				change_notify(chansvs.nick, tu, "You have had protected (+%c) status taken from you "
				                                "on \2%s\2 by \2%s\2", ircd->protect_mchar[1], mc->name,
				                                get_source_name(si));

			if (! si->su || ! chanuser_find(mc->chan, si->su))
				command_success_nodata(si, _("\2%s\2 has had protected (+%c) status taken from them "
				                             "on \2%s\2"), tu->nick, ircd->protect_mchar[1], mc->name);

			logcommand(si, CMDLOG_DO, "DEPROTECT: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost,
			                                                               mc->name);
		}
	}

	prefix_action_clear(&protect_actions);
}

static void
cs_cmd_protect(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PROTECT");
		command_fail(si, fault_needmoreparams, _("Syntax: PROTECT <#channel> [nickname]"));
		return;
	}

	cmd_protect(si, true, parc, parv);
}

static void
cs_cmd_deprotect(struct sourceinfo *si, int parc, char *parv[])
{
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEPROTECT");
		command_fail(si, fault_needmoreparams, _("Syntax: DEPROTECT <#channel> [nickname]"));
		return;
	}

	cmd_protect(si, false, parc, parv);
}

static struct command cs_protect = {
	.name           = "PROTECT",
	.desc           = N_("Gives the channel protection flag to a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_protect,
	.help           = { .path = "cservice/protect" },
};

static struct command cs_deprotect = {
	.name           = "DEPROTECT",
	.desc           = N_("Removes channel protection flag from a user."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_deprotect,
	.help           = { .path = "cservice/protect" },
};

static void
mod_init(struct module *const restrict m)
{
	if (ircd != NULL && !ircd->uses_protect)
	{
		slog(LG_INFO, "Module %s requires protect support, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_protect);
        service_named_bind_command("chanserv", &cs_deprotect);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_protect);
	service_named_unbind_command("chanserv", &cs_deprotect);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/protect", MODULE_UNLOAD_CAPABILITY_OK)
