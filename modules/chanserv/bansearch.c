/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Searches for bans, quiets, and channel modes affecting a user.
 */

#include "atheme.h"

static void
cs_cmd_bansearch_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BANSEARCH");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: BANSEARCH <#channel>"));
		return;
	}

	const char *const channel = parv[0];
	const char *const target = parv[1];

	struct user *tu;

	if (parc == 2)
	{
		if (! has_priv(si, PRIV_USER_AUSPEX))
		{
			(void) command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
			return;
		}
		if (! (tu = user_find_named(target)))
		{
			(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), target);
			return;
		}
	}
	else if (! (tu = si->su))
	{
		(void) command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, "BANSEARCH");
		return;
	}

	struct mychan *const mc = mychan_find(channel);

	if (mc && metadata_find(mc, "private:close:closer"))
	{
		(void) command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
		return;
	}

	struct channel *const c = channel_find(channel);

	if (! c)
	{
		(void) command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, channel);
		return;
	}

	unsigned int count = 0;
	mowgli_node_t *n;

	for (n = next_matching_ban(c, tu, 'b', c->bans.head); n; n = next_matching_ban(c, tu, 'b', n->next))
	{
		if (! count)
			(void) command_success_nodata(si, _("Bans matching \2%s\2 in \2%s\2:"), tu->nick, channel);

		const struct chanban *const cb = n->data;
		count++;

		(void) command_success_nodata(si, "- %s", cb->mask);
	}

	(void) command_success_nodata(si, ngettext(N_("\2%u\2 ban found."), N_("\2%u\2 bans found."), count), count);

	if (! (ircd->type == PROTOCOL_UNREAL || ircd->type == PROTOCOL_INSPIRCD || ircd->type == PROTOCOL_NGIRCD))
	{
		count = 0;

		for (n = next_matching_ban(c, tu, 'q', c->bans.head); n; n = next_matching_ban(c, tu, 'q', n->next))
		{
			if (! count)
				(void) command_success_nodata(si, _("Quiets matching \2%s\2 in \2%s\2:"),
				                              tu->nick, channel);

			const struct chanban *const cb = n->data;
			count++;

			(void) command_success_nodata(si, "- %s", cb->mask);
		}

		(void) command_success_nodata(si, ngettext(N_("\2%u\2 quiet found."), N_("\2%u\2 quiets found."),
		                              count), count);
	}

	if (ircd->type == PROTOCOL_CHARYBDIS && (c->modes & mode_to_flag('r')))
		if (! tu->myuser || (tu->myuser && (tu->myuser->flags & MU_WAITAUTH)))
			(void) command_success_nodata(si, _("\2%s\2 is blocking unidentified users from joining, "
			                                    "and \2%s\2 is not identified."), channel, tu->nick);

	if (c->modes & CMODE_INVITE)
		(void) command_success_nodata(si, _("\2%s\2 is invite-only."), channel);

	if (c->key)
		(void) command_success_nodata(si, _("\2%s\2 has a channel join key."), channel);

	const struct chanuser *const cu = chanuser_find(c, tu);

	if ((c->modes & CMODE_MOD) && cu && ! cu->modes)
		(void) command_success_nodata(si, _("\2%s\2 is moderated and \2%s\2 has no channel privileges."),
		                              channel, tu->nick);
	else if (c->modes & CMODE_MOD)
		(void) command_success_nodata(si, _("\2%s\2 is moderated."), channel);

	if (c->limit && c->nummembers >= c->limit && ! cu)
		(void) command_success_nodata(si, _("\2%s\2 is full."), channel);

	(void) logcommand(si, CMDLOG_GET, "BANSEARCH: \2%s!%s@%s\2 on \2%s\2", tu->nick, tu->user, tu->vhost, channel);
}

static struct command cs_cmd_bansearch = {
	.name		= "BANSEARCH",
	.desc		= N_("Searches for bans, quiets, and other channel modes affecting a user."),
	.access		= AC_NONE,
	.maxparc	= 2,
	.cmd		= &cs_cmd_bansearch_func,
	.help		= { .path = "cservice/bansearch" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	(void) service_named_bind_command("chanserv", &cs_cmd_bansearch);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("chanserv", &cs_cmd_bansearch);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/bansearch", MODULE_UNLOAD_CAPABILITY_OK)
