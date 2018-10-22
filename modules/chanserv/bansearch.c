/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Searches for bans, quiets, and channel modes affecting a user.
 */

#include "atheme.h"

static void
cs_cmd_bansearch(struct sourceinfo *si, int parc, char *parv[])
{
	const char *channel = parv[0];
	const char *target  = parv[1];

	if (!channel)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BANSEARCH");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: BANSEARCH <#channel>"));
		return;
	}

	struct user *tu;

	if (target)
	{
		if (!has_priv(si, PRIV_USER_AUSPEX))
		{
			(void) command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
			return;
		}

		tu = user_find_named(target);

		if (tu == NULL)
		{
			(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), target);
			return;
		}
	}
	else
	{
		if (si->su == NULL)
		{
			(void) command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "BANSEARCH");
			return;
		}

		tu = si->su;
	}

	struct channel *c = channel_find(channel);

	if (!c)
	{
		(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), channel);
		return;
	}

	mowgli_node_t *n, *tn;
	struct chanban *cb;
	int count = 0;

	(void) command_success_nodata(si, _("Bans matching \2%s\2 in \2%s\2"), tu->nick, channel);
	for (n = next_matching_ban(c, tu, 'b', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'b', tn))
	{
		tn = n->next;
		cb = n->data;

		(void) command_success_nodata(si, "- %s", cb->mask);
		count++;
	}
	(void) command_success_nodata(si, ngettext(N_("\2%d\2 ban found"), N_("\2%d\2 bans found"), count), count);

	if (!(ircd->type == PROTOCOL_UNREAL || ircd->type == PROTOCOL_INSPIRCD || ircd->type == PROTOCOL_NGIRCD))
	{
		count = 0;
		(void) command_success_nodata(si, _("Quiets matching \2%s\2 in \2%s\2"), tu->nick, channel);
		for (n = next_matching_ban(c, tu, 'q', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'q', tn))
		{
			tn = n->next;
			cb = n->data;

			(void) command_success_nodata(si, "- %s", cb->mask);
			count++;
		}
		(void) command_success_nodata(si, ngettext(N_("\2%d\2 quiet found"), N_("\2%d\2 quiets found"), count), count);
	}

	if (c->modes & CMODE_INVITE)
	{
		(void) command_success_nodata(si, _("\2%s\2 is invite-only."), channel);
	}

	if (c->key)
	{
		(void) command_success_nodata(si, _("\2%s\2 has a password."), channel);
	}

	if ((c->modes & mode_to_flag('r')) && ircd->type == PROTOCOL_CHARYBDIS)
	{
		if (tu->myuser == NULL || (tu->myuser != NULL && (tu->myuser->flags & MU_WAITAUTH)))
		{
			(void) command_success_nodata(si, _("\2%s\2 is blocking unidentified users and \2%s\2 is not identified."), channel, tu->nick);
		}
	}

	if (c->modes & CMODE_MOD)
	{
		struct chanuser *cu;

		cu = chanuser_find(c, tu);

		if (!cu)
		{
			(void) command_success_nodata(si, _("\2%s\2 is moderated and \2%s\2 might not be voiced or opped."), channel, tu->nick);
		}
		else if (!cu->modes)
		{
			(void) command_success_nodata(si, _("\2%s\2 is moderated and \2%s\2 is not voiced or opped."), channel, tu->nick);
		}
	}

	if (c->limit != 0 && c->nummembers >= c->limit)
	{
		(void) command_success_nodata(si, _("\2%s\2 is full."), channel);
	}

	(void) logcommand(si, CMDLOG_GET, "BANSEARCH: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost, channel);
}

static struct command cs_bansearch = {
	.name		= "BANSEARCH",
	.desc		= N_("Searches for bans, quiets, and channel modes affecting a user."),
	.access		= AC_NONE,
	.maxparc	= 2,
	.cmd		= &cs_cmd_bansearch,
	.help		= { .path = "cservice/bansearch" },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	(void) service_named_bind_command("chanserv", &cs_bansearch);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cs_bansearch);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/bansearch", MODULE_UNLOAD_CAPABILITY_OK)
