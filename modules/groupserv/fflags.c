/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_fflags(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	struct mygroup *mg;
	struct myentity *mt;
	struct groupacs *ga;
	unsigned int flags = 0;

	if (!parv[0] || !parv[1] || !parv[2])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FFLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: FFLAGS <!group> [user] [changes]"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}

        if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
		return;
	}

	if ((mt = myentity_find_ext(parv[1])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), parv[1]);
		return;
	}

	ga = groupacs_find(mg, mt, 0, false);
	if (ga != NULL)
		flags = ga->flags;

	flags = gs_flags_parser(parv[2], 1, flags);

	if (!(flags & GA_FOUNDER) && groupacs_find(mg, mt, GA_FOUNDER, false))
	{
		if (mygroup_count_flag(mg, GA_FOUNDER) == 1)
		{
			command_fail(si, fault_noprivs, _("You may not remove the last founder."));
			return;
		}

		if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
		{
			command_fail(si, fault_noprivs, _("You may not remove a founder's +F access."));
			return;
		}
	}

	if (ga != NULL && flags != 0)
		ga->flags = flags;
	else if (ga != NULL)
	{
		groupacs_delete(mg, mt);
		command_success_nodata(si, _("\2%s\2 has been removed from \2%s\2."), mt->name, entity(mg)->name);
		wallops("\2%s\2 is removing flags for \2%s\2 on \2%s\2", get_oper_name(si), mt->name, entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "FFLAGS:REMOVE: \2%s\2 on \2%s\2", mt->name, entity(mg)->name);
		return;
	}
	else
	{
		if (MOWGLI_LIST_LENGTH(&mg->acs) > gs_config->maxgroupacs && (!(mg->flags & MG_ACSNOLIMIT)))
		{
			command_fail(si, fault_toomany, _("Group \2%s\2 access list is full."), entity(mg)->name);
			return;
		}
		ga = groupacs_add(mg, mt, flags);
	}

	MOWGLI_ITER_FOREACH(n, entity(mg)->chanacs.head)
	{
		struct chanacs *ca = n->data;

		verbose(ca->mychan, "\2%s\2 now has flags \2%s\2 in the group \2%s\2 which communally has \2%s\2 on \2%s\2.",
			mt->name, gflags_tostr(ga_flags, ga->flags), entity(mg)->name,
			bitmask_to_flags(ca->level), ca->mychan->name);

		hook_call_channel_acl_change(&(struct hook_channel_acl_req){ .ca = ca });
	}

	command_success_nodata(si, _("\2%s\2 now has flags \2%s\2 on \2%s\2."), mt->name, gflags_tostr(ga_flags, ga->flags), entity(mg)->name);

	// XXX
	wallops("\2%s\2 is modifying flags(\2%s\2) for \2%s\2 on \2%s\2", get_oper_name(si), gflags_tostr(ga_flags, ga->flags), mt->name, entity(mg)->name);
	logcommand(si, CMDLOG_ADMIN, "FFLAGS: \2%s\2 now has flags \2%s\2 on \2%s\2", mt->name, gflags_tostr(ga_flags,  ga->flags), entity(mg)->name);
}

static struct command gs_fflags = {
	.name           = "FFLAGS",
	.desc           = N_("Forces a flag change on a user in a group."),
	.access         = PRIV_GROUP_ADMIN,
	.maxparc        = 3,
	.cmd            = &gs_cmd_fflags,
	.help           = { .path = "groupserv/fflags" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_fflags);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_fflags);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/fflags", MODULE_UNLOAD_CAPABILITY_OK)
