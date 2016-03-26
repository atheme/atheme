/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 *
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/fflags", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

static void gs_cmd_fflags(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_fflags = { "FFLAGS", N_("Forces a flag change on a user in a group."), PRIV_GROUP_ADMIN, 3, gs_cmd_fflags, { .path = "groupserv/fflags" } };

static void gs_cmd_fflags(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	mygroup_t *mg;
	myentity_t *mt;
	groupacs_t *ga;
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
		command_fail(si, fault_noprivs, _("You are not logged in."));
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
			command_fail(si, fault_toomany, _("Group %s access list is full."), entity(mg)->name);
			return;
		}
		ga = groupacs_add(mg, mt, flags);
	}

	MOWGLI_ITER_FOREACH(n, entity(mg)->chanacs.head)
	{
		chanacs_t *ca = n->data;

		verbose(ca->mychan, "\2%s\2 now has flags \2%s\2 in the group \2%s\2 which communally has \2%s\2 on \2%s\2.",
			mt->name, gflags_tostr(ga_flags, ga->flags), entity(mg)->name,
			bitmask_to_flags(ca->level), ca->mychan->name);

		hook_call_channel_acl_change(&(hook_channel_acl_req_t){ .ca = ca });
	}

	command_success_nodata(si, _("\2%s\2 now has flags \2%s\2 on \2%s\2."), mt->name, gflags_tostr(ga_flags, ga->flags), entity(mg)->name);

	/* XXX */
	wallops("\2%s\2 is modifying flags(\2%s\2) for \2%s\2 on \2%s\2", get_oper_name(si), gflags_tostr(ga_flags, ga->flags), mt->name, entity(mg)->name);
	logcommand(si, CMDLOG_ADMIN, "FFLAGS: \2%s\2 now has flags \2%s\2 on \2%s\2", mt->name, gflags_tostr(ga_flags,  ga->flags), entity(mg)->name);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_fflags);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_fflags);
}

