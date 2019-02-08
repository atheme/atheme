/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"
#include "groupserv.h"

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_invite_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INVITE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: INVITE <DELETE|SHOW|!group [user]>"));
		return;
	}

	if (strcasecmp(parv[0], "DELETE") == 0 || strcasecmp(parv[0], "SHOW") == 0)
	{
		const struct metadata *const md = metadata_find(si->smu, "private:groupinvite");

		if (! md)
		{
			(void) command_success_nodata(si, _("You have no pending group invite."));
			return;
		}

		const char *const group = md->value;

		if (parv[0][0] == 'D')
		{
			(void) metadata_delete(si->smu, "private:groupinvite");
			(void) logcommand(si, CMDLOG_GET, "INVITE:DELETE");
			(void) command_success_nodata(si, _("Your pending group invite to \2%s\2 has been deleted."),
			                              group);
		}
		else
		{
			(void) logcommand(si, CMDLOG_GET, "INVITE:SHOW");
			(void) command_success_nodata(si, _("You have a pending group invite to \2%s\2."), group);
		}

		return;
	}

	if (parc != 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INVITE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: INVITE <DELETE|SHOW|!group [user]>"));
		return;
	}

	const char *const group = parv[0];
	const char *const param = parv[1];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "INVITE");
		(void) command_fail(si, fault_badparams, _("Syntax: INVITE <DELETE|SHOW|!group [user]>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_INVITE))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	struct myuser *mu;

	if (! (mu = myuser_find_ext(param)))
	{
		(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), param);
		return;
	}

	struct groupacs *ga;

	if ((ga = gcsyms->groupacs_find(mg, entity(mu), 0, false)))
	{
		(void) command_fail(si, fault_badparams, _("\2%s\2 is already a member of \2%s\2."), param, group);
		return;
	}

	if (metadata_find(mu, "private:groupinvite"))
	{
		(void) command_fail(si, fault_badparams, _("\2%s\2 can not be invited to a group currently because "
		                                           "they already have another invitation pending."), param);
		return;
	}

	if (mu->flags & MU_NEVERGROUP)
	{
		(void) command_fail(si, fault_noprivs, _("\2%s\2 does not wish to belong to any groups."), param);
		return;
	}

	(void) metadata_add(mu, "private:groupinvite", group);

	struct service *svs;

	if ((svs = service_find("memoserv")))
	{
		char buf[BUFSIZE];

		(void) snprintf(buf, sizeof buf, "%s [auto memo] You have been invited to the group \2%s\2.",
		                param, group);

		(void) command_exec_split(svs, si, "SEND", buf, svs->commands);
	}
	else
	{
		(void) myuser_notice(si->service->nick, mu, "You have been invited to the group \2%s\2.", group);
	}

	(void) logcommand(si, CMDLOG_SET, "INVITE: \2%s\2 \2%s\2", group, param);
	(void) command_success_nodata(si, _("\2%s\2 has been invited to \2%s\2"), param, group);
}

static struct command gs_cmd_invite = {
	.name           = "INVITE",
	.desc           = N_("Invites a user to a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_invite_func,
	.help           = { .path = "groupserv/invite" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_invite);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_invite);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/invite", MODULE_UNLOAD_CAPABILITY_OK)
