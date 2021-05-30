/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * This file contains code for the CService KICK functions.
 */

#include <atheme.h>

static void
cs_cmd_kick(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	char *reason = parv[2];
	struct mychan *mc;
	struct user *tu;
	struct chanuser *cu;
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KICK");
		command_fail(si, fault_needmoreparams, _("Syntax: KICK <#channel> <nickname> [reason]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
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

	// figure out who we're going to kick
	if ((tu = user_find_named(nick)) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
		return;
	}

	// if target is a service, bail. --nenolod
	if (is_service(tu))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is a network service; you cannot kick or deop them."), tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	if (cu->modes & CSTATUS_OWNER || cu->modes & CSTATUS_PROTECT)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is protected from kicks; you cannot kick them."), tu->nick);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "(%s) %s", get_source_name(si), reason ? reason : "No reason given");
	try_kick(chansvs.me->me, mc->chan, tu, reasonbuf);
	logcommand(si, CMDLOG_DO, "KICK: \2%s!%s@%s\2 from \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (si->su == NULL ||
			(si->su != tu && !chanuser_find(mc->chan, si->su)))
		command_success_nodata(si, _("\2%s\2 has been kicked from \2%s\2."), tu->nick, mc->name);
}

static void
cs_cmd_kickban(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	char *reason = parv[2];
	struct mychan *mc;
	struct user *tu;
	struct chanuser *cu;
	char reasonbuf[BUFSIZE];
	unsigned int n;

	if (!chan || !nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "KICKBAN");
		command_fail(si, fault_needmoreparams, _("Syntax: KICKBAN <#channel> <nickname> [reason]"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_REMOVE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	// figure out who we're going to kick
	if ((tu = user_find_named(nick)) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
		return;
	}

        // if target is a service, bail. --nenolod
	if (is_service(tu))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is a network service; you cannot kick or deop them."), tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
		return;
	}

	if (cu->modes & CSTATUS_OWNER || cu->modes & CSTATUS_PROTECT)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is protected from kicks; you cannot kick them."), tu->nick);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "(%s) %s", get_source_name(si), reason ? reason : "No reason given");
	ban(si->service->me, mc->chan, tu);
	n = remove_ban_exceptions(si->service->me, mc->chan, tu);
	if (n > 0)
		command_success_nodata(si, ngettext(N_("To avoid rejoin, %u ban exception matching \2%s\2 has been removed from \2%s\2."),
		                                    N_("To avoid rejoin, %u ban exceptions matching \2%s\2 have been removed from \2%s\2."),
		                                    n), n, tu->nick, mc->name);

	try_kick(chansvs.me->me, mc->chan, tu, reasonbuf);
	logcommand(si, CMDLOG_DO, "KICKBAN: \2%s!%s@%s\2 from \2%s\2", tu->nick, tu->user, tu->vhost, mc->name);
	if (si->su == NULL ||
			(si->su != tu && !chanuser_find(mc->chan, si->su)))
		command_success_nodata(si, _("\2%s\2 has been kickbanned from \2%s\2."), tu->nick, mc->name);
}

static struct command cs_kick = {
	.name           = "KICK",
	.desc           = N_("Removes a user from a channel."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_kick,
	.help           = { .path = "cservice/kick" },
};

static struct command cs_kickban = {
	.name           = "KICKBAN",
	.desc           = N_("Removes and bans a user from a channel."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_kickban,
	.help           = { .path = "cservice/kickban" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_kick);
	service_named_bind_command("chanserv", &cs_kickban);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_kick);
	service_named_unbind_command("chanserv", &cs_kickban);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/kick", MODULE_UNLOAD_CAPABILITY_OK)
