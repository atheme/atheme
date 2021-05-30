/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (C) 2014-2017 Austin Ellis (siniStar[at]IRC4Fun.net)
 *
 * This file contains code for the CService INVITE functions.
 */

#include <atheme.h>

static void
cs_cmd_invite(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	struct mychan *mc;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, STR_IRC_COMMAND_ONLY, "INVITE");
		return;
	}

	/* This command is not useful if the user is already in the channel,
	 * ignore it if it is a fantasy command so users can program bots to
	 * react on it without interference from ChanServ.
	 */
	if (si->c != NULL)
		return;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INVITE");
		command_fail(si, fault_needmoreparams, _("Syntax: INVITE <#channel>"));
		return;
	}

	if (!(mc = mychan_find(chan)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_INVITE))
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
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, mc->name);
		return;
	}

	if (chanuser_find(mc->chan, si->su))
	{
		command_fail(si, fault_noprivs, _("You're already on \2%s\2."), mc->name);
		return;
	}

	invite_sts(si->service->me, si->su, mc->chan);
	logcommand(si, CMDLOG_DO, "INVITE: \2%s\2", mc->name);
	command_success_nodata(si, _("You have been invited to \2%s\2."), mc->name);
}

static struct command cs_invite = {
	.name           = "INVITE",
	.desc           = N_("Invites you to a channel."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_invite,
	.help           = { .path = "cservice/invite" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_invite);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_invite);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/invite", MODULE_UNLOAD_CAPABILITY_OK)
