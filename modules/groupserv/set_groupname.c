/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2014 William Pitcock <nenolod -at- nenolod.net>
 *
 * Changes the group name
 */

#include <atheme.h>
#include "groupserv.h"

// SET GROUPNAME <name>
static void
gs_cmd_set_groupname(struct sourceinfo *si, int parc, char *parv[])
{
	char *oldname, *newname;
	struct mygroup *mg;

	if (parc != 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET GROUPNAME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET GROUPNAME <oldname> <newname>"));
		return;
	}

	oldname = parv[0];
	newname = parv[1];

	if (*oldname != '!' || *newname != '!')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET GROUPNAME");
		command_fail(si, fault_badparams, _("Syntax: SET GROUPNAME <oldname> <newname>"));
		return;
	}

	mg = mygroup_find(oldname);
	if (mg == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), oldname);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (strcmp(entity(mg)->name, newname) == 0)
	{
		command_fail(si, fault_nochange, _("The group name is already set to \2%s\2."), newname);
		return;
	}

	if (mygroup_find(newname) != NULL)
	{
		command_fail(si, fault_nochange, _("The group \2%s\2 already exists."), newname);
		return;
	}

	mygroup_rename(mg, newname);

	logcommand(si, CMDLOG_REGISTER, "SET:GROUPNAME: \2%s\2 to \2%s\2", oldname, newname);
	command_success_nodata(si, _("The group \2%s\2 has been renamed to \2%s\2."), oldname, newname);
}

static struct command gs_set_groupname = {
	.name           = "GROUPNAME",
	.desc           = N_("Changes the group's name."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &gs_cmd_set_groupname,
	.help           = { .path = "groupserv/set_groupname" },
};

static void
mod_init(struct module *const restrict m)
{
        use_groupserv_main_symbols(m);
        use_groupserv_set_symbols(m);

	command_add(&gs_set_groupname, gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&gs_set_groupname, gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_groupname", MODULE_UNLOAD_CAPABILITY_OK)
