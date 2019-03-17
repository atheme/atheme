/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_fflags_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 3)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FFLAGS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: FFLAGS <!group> <target> <flags>"));
		return;
	}

	const char *const group = parv[0];
	const char *const param = parv[1];
	const char *const flags = parv[2];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FFLAGS");
		(void) command_fail(si, fault_badparams, _("Syntax: FFLAGS <!group> <target> <flags>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	struct myentity *mt;

	if (! (mt = myentity_find_ext(param)))
	{
		(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), param);
		return;
	}

	struct groupacs *ga = gcsyms->groupacs_find(mg, mt, 0, false);
	unsigned int gaflags = 0;

	if (ga)
		gaflags = ga->flags;

	gaflags = gcsyms->gs_flags_parser(flags, true, gaflags);

	if (! (gaflags & GA_FOUNDER) && gcsyms->groupacs_find(mg, mt, GA_FOUNDER, false))
	{
		if (gcsyms->mygroup_count_flag(mg, GA_FOUNDER) == 1)
		{
			(void) command_fail(si, fault_noprivs, _("You may not remove the last founder."));
			return;
		}
		if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
		{
			(void) command_fail(si, fault_noprivs, _("You may not remove a founder's +F access."));
			return;
		}
	}

	if (ga && gaflags)
	{
		if (ga->flags != gaflags)
		{
			ga->flags = gaflags;
		}
		else
		{
			(void) command_fail(si, fault_nochange, _("Group \2%s\2 access for \2%s\2 unchanged."), group, param);
			return;
		}
	}
	else if (ga)
	{
		(void) gcsyms->groupacs_delete(mg, mt);
		(void) command_success_nodata(si, _("\2%s\2 has been removed from \2%s\2."), param, group);
		(void) wallops("\2%s\2 is removing flags for \2%s\2 on \2%s\2", get_oper_name(si), param, group);
		(void) logcommand(si, CMDLOG_ADMIN, "FFLAGS:REMOVE: \2%s\2 on \2%s\2", param, group);
		return;
	}
	else if (gaflags)
	{
		if (MOWGLI_LIST_LENGTH(&mg->acs) > gcsyms->config->maxgroupacs && ! (mg->flags & MG_ACSNOLIMIT))
		{
			(void) command_fail(si, fault_toomany, _("Group \2%s\2 access list is full."), group);
			return;
		}

		ga = gcsyms->groupacs_add(mg, mt, gaflags);
	}
	else
	{
		(void) command_fail(si, fault_nochange, _("Group \2%s\2 access for \2%s\2 unchanged."), group, param);
		return;
	}

	const char *const sflags = gflags_tostr(ga_flags, ga->flags);

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, entity(mg)->chanacs.head)
	{
		struct chanacs *const ca = n->data;
		const struct mychan *const mc = ca->mychan;

		(void) verbose(mc, "\2%s\2 now has flags \2%s\2 in the group \2%s\2 which communally has \2%s\2 "
		               "on \2%s\2.", param, sflags, group, bitmask_to_flags(ca->level), mc->name);

		(void) hook_call_channel_acl_change(&(hook_channel_acl_req_t){ .ca = ca });
	}

	(void) command_success_nodata(si, _("\2%s\2 now has flags \2%s\2 on \2%s\2."), param, sflags, group);
	(void) wallops("\2%s\2 is modifying flags (\2%s\2) for \2%s\2 on \2%s\2", get_oper_name(si), sflags, param, group);
	(void) logcommand(si, CMDLOG_ADMIN, "FFLAGS: \2%s\2 now has flags \2%s\2 on \2%s\2", param, sflags, group);
}

static struct command gs_cmd_fflags = {
	.name           = "FFLAGS",
	.desc           = N_("Forces a flag change on a user in a group."),
	.access         = PRIV_GROUP_ADMIN,
	.maxparc        = 3,
	.cmd            = &gs_cmd_fflags_func,
	.help           = { .path = "groupserv/fflags" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_fflags);

	(void) hook_add_event("channel_acl_change");
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_fflags);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/fflags", MODULE_UNLOAD_CAPABILITY_OK)
