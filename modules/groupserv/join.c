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
gs_cmd_join(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	struct metadata *md, *md2;
	unsigned int flags = 0;
	bool invited = false;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOIN");
		command_fail(si, fault_needmoreparams, _("Syntax: JOIN <!groupname>"));
		return;
	}

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_alreadyexists, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}

	if ((md2 = metadata_find(si->smu, "private:groupinvite")))
	{
		if (!strcasecmp(md2->value, parv[0]))
			invited = true;
		else
			invited = false;
	}

	if (!(mg->flags & MG_OPEN) && !invited)
	{
		command_fail(si, fault_noprivs, _("Group \2%s\2 is not open to anyone joining."), parv[0]);
		return;
	}

	if (groupacs_sourceinfo_has_flag(mg, si, GA_BAN))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (groupacs_sourceinfo_has_flag(mg, si, 0))
	{
		command_fail(si, fault_nochange, _("You are already a member of group \2%s\2."), parv[0]);
		return;
	}

	if (MOWGLI_LIST_LENGTH(&mg->acs) > gs_config->maxgroupacs && (!(mg->flags & MG_ACSNOLIMIT)) && !invited)
        {
                command_fail(si, fault_toomany, _("Group \2%s\2 access list is full."), entity(mg)->name);
                return;
        }

	if ((md = metadata_find(mg, "joinflags")))
		flags = atoi(md->value);
	else
		flags = gs_flags_parser(gs_config->join_flags, 0, flags);

	groupacs_add(mg, entity(si->smu), flags);

	if (invited)
		metadata_delete(si->smu, "private:groupinvite");

	command_success_nodata(si, _("You are now a member of \2%s\2."), entity(mg)->name);
}

static struct command gs_join = {
	.name           = "JOIN",
	.desc           = N_("Join a open group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_join,
	.help           = { .path = "groupserv/join" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_join);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_join);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/join", MODULE_UNLOAD_CAPABILITY_OK)
