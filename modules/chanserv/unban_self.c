/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 William Pitcock, et al.
 *
 * This file contains a CService UNBAN which can only unbans the source
 * user, not others.
 * Do not load chanserv/ban and chanserv/unban_self together.
 */

#include <atheme.h>

static void
cs_cmd_unban(struct sourceinfo *si, int parc, char *parv[])
{
        const char *channel = parv[0];
        const char *target = parv[1];
        struct channel *c = channel_find(channel);
	struct mychan *mc = mychan_find(channel);
	struct user *tu;
	struct chanban *cb;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, "UNBAN");
		return;
	}

	if (!channel)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "UNBAN");
		command_fail(si, fault_needmoreparams, _("Syntax: UNBAN <#channel>"));
		return;
	}

	if (target && irccasecmp(target, si->su->nick))
	{
		command_fail(si, fault_noprivs, _("You may only unban yourself via %s."), si->service->nick);
		if (validhostmask(target))
			command_fail(si, fault_noprivs, _("Try \2/mode %s -b %s\2"), channel, target);
		return;
	}
	target = si->su->nick;

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE) &&
			!chanacs_source_has_flag(mc, si, CA_EXEMPT))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, channel);
		return;
	}

	if (!c)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, channel);
		return;
	}

	tu = si->su;
	{
		mowgli_node_t *n, *tn;
		char hostbuf2[BUFSIZE];
		unsigned int count = 0;

		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);
		for (n = next_matching_ban(c, tu, 'b', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'b', tn))
		{
			tn = n->next;
			cb = n->data;

			logcommand(si, CMDLOG_DO, "UNBAN: \2%s\2 \2%s\2 (for user \2%s\2)", mc->name, cb->mask, hostbuf2);
			modestack_mode_param(chansvs.nick, c, MTYPE_DEL, cb->type, cb->mask);
			chanban_delete(cb);
			count++;
		}
		if (count > 0)
			command_success_nodata(si, ngettext(N_("Unbanned \2%s\2 on \2%s\2 (%u ban removed)."),
			                                    N_("Unbanned \2%s\2 on \2%s\2 (%u bans removed)."),
			                                    count), target, channel, count);
		else
			command_success_nodata(si, _("No bans found matching \2%s\2 on \2%s\2."), target, channel);
		return;
	}
}

static struct command cs_unban = {
	.name           = "UNBAN",
	.desc           = N_("Unbans you on a channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &cs_cmd_unban,
	.help           = { .path = "cservice/unban_self" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_CONFLICT(m, "chanserv/ban")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_unban);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_unban);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/unban_self", MODULE_UNLOAD_CAPABILITY_OK)
