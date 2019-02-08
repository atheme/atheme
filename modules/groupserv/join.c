/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_join_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOIN");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: JOIN <!group>"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "JOIN");
		(void) command_fail(si, fault_badparams, _("Syntax: JOIN <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_alreadyexists, _("Group \2%s\2 does not exist."), group);
		return;
	}

	if (gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_BAN))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}
	if (gcsyms->groupacs_sourceinfo_has_flag(mg, si, 0))
	{
		(void) command_fail(si, fault_nochange, _("You are already a member of group \2%s\2."), group);
		return;
	}

	const struct metadata *md;
	bool invited = false;

	if ((md = metadata_find(si->smu, "private:groupinvite")) && strcasecmp(md->value, group) == 0)
		invited = true;

	if (! (mg->flags & MG_OPEN) && ! invited)
	{
		(void) command_fail(si, fault_noprivs, _("Group \2%s\2 is not open to anyone joining."), group);
		return;
	}
	if (MOWGLI_LIST_LENGTH(&mg->acs) > gcsyms->config->maxgroupacs && ! (mg->flags & MG_ACSNOLIMIT) && ! invited)
	{
		(void) command_fail(si, fault_toomany, _("Group \2%s\2 access list is full."), group);
		return;
	}

	if (invited)
		(void) metadata_delete(si->smu, "private:groupinvite");

	unsigned int flags;

	if ((md = metadata_find(mg, "joinflags")))
		flags = atoi(md->value);
	else
		flags = gcsyms->gs_flags_parser(gcsyms->config->join_flags, true, 0);

	(void) gcsyms->groupacs_add(mg, entity(si->smu), flags);
	(void) command_success_nodata(si, _("You are now a member of \2%s\2."), group);
}

static struct command gs_cmd_join = {
	.name           = "JOIN",
	.desc           = N_("Join a open group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_join_func,
	.help           = { .path = "groupserv/join" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_join);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_join);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/join", MODULE_UNLOAD_CAPABILITY_OK)
