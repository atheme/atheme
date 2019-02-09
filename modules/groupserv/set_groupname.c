/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2014 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static const struct groupserv_core_symbols *gcsyms = NULL;
static mowgli_patricia_t **gs_set_cmdtree = NULL;

static void
gs_cmd_set_groupname_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET GROUPNAME");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET GROUPNAME <!oldname> <!newname>"));
		return;
	}

	const char *const oldname = parv[0];
	const char *const newname = parv[1];

	if (*oldname != '!' || *newname != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET GROUPNAME");
		(void) command_fail(si, fault_badparams, _("Syntax: SET GROUPNAME <!oldname> <!newname>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(oldname)))
	{
		(void) command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), oldname);
		return;
	}

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (strcmp(entity(mg)->name, newname) == 0)
	{
		(void) command_fail(si, fault_nochange, _("The group name is already set to \2%s\2."), newname);
		return;
	}

	if (gcsyms->mygroup_find(newname))
	{
		(void) command_fail(si, fault_nochange, _("The group \2%s\2 already exists."), newname);
		return;
	}

	(void) gcsyms->mygroup_rename(mg, newname);

	(void) logcommand(si, CMDLOG_SET, "SET:GROUPNAME: \2%s\2 to \2%s\2", oldname, newname);
	(void) command_success_nodata(si, _("The group \2%s\2 has been renamed to \2%s\2."), oldname, newname);
}

static struct command gs_cmd_set_groupname = {
	.name           = "GROUPNAME",
	.desc           = N_("Changes the group's name."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_groupname_func,
	.help           = { .path = "groupserv/set_groupname" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");
	MODULE_TRY_REQUEST_SYMBOL(m, gs_set_cmdtree, "groupserv/set_core", "gs_set_cmdtree");

	(void) command_add(&gs_cmd_set_groupname, *gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&gs_cmd_set_groupname, *gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_groupname", MODULE_UNLOAD_CAPABILITY_OK)
