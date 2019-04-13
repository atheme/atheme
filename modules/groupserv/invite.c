/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the GroupServ INVITE command.
 */

// TODO: We should probably add a way for the target user to remove pending invites

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_invite(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	struct myuser *mu;
	struct groupacs *ga;
	char *group = parv[0];
	char *user = parv[1];
	char buf[BUFSIZE];
	struct service *svs;

	if (!group || !user)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INVITE");
		command_fail(si, fault_needmoreparams, _("Syntax: INVITE <!group> <user>"));
		return;
	}

	if ((mg = mygroup_find(group)) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_INVITE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if ((mu = myuser_find_ext(user)) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), user);
		return;
	}

	if ((ga = groupacs_find(mg, entity(mu), 0, false)) != NULL)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is already a member of \2%s\2."), user, group);
		return;
	}

	if (metadata_find(mu, "private:groupinvite"))
	{
		command_fail(si, fault_badparams, _("\2%s\2 can not be invited to a group currently because they already \
					have another invitation pending."), user);
		return;
	}

	if (MU_NEVERGROUP & mu->flags)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 does not wish to belong to any groups."), user);
		return;
	}

	metadata_add(mu, "private:groupinvite", group);

	if ((svs = service_find("memoserv")) != NULL)
	{
		snprintf(buf, BUFSIZE, "%s [auto memo] You have been invited to the group \2%s\2.", user, group);

		command_exec_split(svs, si, "SEND", buf, svs->commands);
	}
	else
	{
		myuser_notice(si->service->nick, mu, "You have been invited to the group \2%s\2.", group);
	}

	logcommand(si, CMDLOG_SET, "INVITE: \2%s\2 \2%s\2", group, user);
	command_success_nodata(si, _("\2%s\2 has been invited to \2%s\2"), user, group);
}

static struct command gs_invite = {
	.name           = "INVITE",
	.desc           = N_("Invites a user to a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_invite,
	.help           = { .path = "groupserv/invite" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_invite);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_invite);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/invite", MODULE_UNLOAD_CAPABILITY_OK)
