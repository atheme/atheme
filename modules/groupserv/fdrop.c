/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include "atheme.h"
#include "groupserv.h"

static void gs_cmd_fdrop(struct sourceinfo *si, int parc, char *parv[]);

static struct command gs_fdrop = { "FDROP", N_("Force drops a group registration."), PRIV_GROUP_ADMIN, 1, gs_cmd_fdrop, { .path = "groupserv/fdrop" } };

static void
gs_cmd_fdrop(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	char *name = parv[0];

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (*name != '!')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (!(mg = mygroup_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), name);
		return;
	}

	remove_group_chanacs(mg);
	logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FDROP: \2%s\2", entity(mg)->name);
        wallops("%s dropped the group \2%s\2", get_oper_name(si), name);
	atheme_object_unref(mg);
	command_success_nodata(si, _("The group \2%s\2 has been dropped."), name);
	return;
}

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_fdrop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_fdrop);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/fdrop", MODULE_UNLOAD_CAPABILITY_OK)
