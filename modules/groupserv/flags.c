/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"
#include "groupserv.h"

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_flags_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <!group> [target [flags]]"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FLAGS");
		(void) command_fail(si, fault_badparams, _("Syntax: FLAGS <!group> [target [flags]]"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	bool operoverride = false;

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, ((parc == 3) ? GA_FLAGS : GA_ACLVIEW)))
	{
		if (has_priv(si, PRIV_GROUP_AUSPEX))
		{
			operoverride = true;
		}
		else
		{
			(void) command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
	}

	const char *const param = parv[1];

	if (! param)
	{
		(void) command_success_nodata(si, _("Entry Account                Flags"));
		(void) command_success_nodata(si, _("----- ---------------------- -----"));

		mowgli_node_t *n;
		int i = 1;

		MOWGLI_ITER_FOREACH(n, mg->acs.head)
		{
			const struct groupacs *const ga = n->data;

			(void) command_success_nodata(si, _("%-5d %-22s %s"), i, ga->mt->name,
			                              gflags_tostr(ga_flags, ga->flags));

			i++;
		}

		(void) command_success_nodata(si, _("----- ---------------------- -----"));
		(void) command_success_nodata(si, _("End of \2%s\2 FLAGS listing."), group);

		if (operoverride)
			(void) logcommand(si, CMDLOG_ADMIN, "FLAGS: \2%s\2 (oper override)", group);
		else
			(void) logcommand(si, CMDLOG_GET, "FLAGS: \2%s\2", group);

		return;
	}

	// simple check since it's already checked above
	if (operoverride)
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	struct myentity *mt;

	if (! (mt = myentity_find_ext(param)))
	{
		(void) command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), param);
		return;
	}

	const char *const flags = parv[2];

	if (! flags)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FLAGS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: FLAGS <!group> <target> <flags>"));
		return;
	}

	if (isuser(mt) && (user(mt)->flags & MU_NEVERGROUP) && ! gcsyms->groupacs_find(mg, mt, 0, true))
	{
		(void) command_fail(si, fault_noprivs, _("\2%s\2 does not wish to have flags in any groups."), param);
		return;
	}

	struct groupacs *ga = gcsyms->groupacs_find(mg, mt, 0, false);
	unsigned int gaflags = 0;

	if (ga)
		gaflags = ga->flags;

	unsigned int oldflags = gaflags;

	gaflags = gcsyms->gs_flags_parser(flags, true, gaflags);

	// check for MU_NEVEROP and forbid committing the change if it's enabled
	if (! (oldflags & GA_CHANACS) && (gaflags & GA_CHANACS))
	{
		if (isuser(mt) && (user(mt)->flags & MU_NEVEROP))
		{
			(void) command_fail(si, fault_noprivs, _("\2%s\2 does not wish to be added to channel access "
			                                         "lists (NEVEROP set)."), param);
			return;
		}
	}

	if ((gaflags & GA_FOUNDER) && ! (oldflags & GA_FOUNDER))
	{
		if (! (gcsyms->groupacs_sourceinfo_flags(mg, si) & GA_FOUNDER))
		{
			gaflags &= ~GA_FOUNDER;
			goto no_founder;
		}

		gaflags |= GA_FLAGS;
	}
	else if ((oldflags & GA_FOUNDER) && ! (gaflags & GA_FOUNDER) && ! (gcsyms->groupacs_sourceinfo_flags(mg, si) & GA_FOUNDER))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

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

no_founder:
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
		(void) logcommand(si, CMDLOG_SET, "FLAGS:REMOVE: \2%s\2 on \2%s\2", param, group);
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
	(void) logcommand(si, CMDLOG_SET, "FLAGS: \2%s\2 now has flags \2%s\2 on \2%s\2", param, sflags, group);
}

static struct command gs_cmd_flags = {
	.name           = "FLAGS",
	.desc           = N_("Sets flags on a user in a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 3,
	.cmd            = &gs_cmd_flags_func,
	.help           = { .path = "groupserv/flags" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_flags);

	(void) hook_add_event("channel_acl_change");
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_flags);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/flags", MODULE_UNLOAD_CAPABILITY_OK)
