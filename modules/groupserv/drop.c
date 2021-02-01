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
gs_cmd_drop_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char *parv[])
{
	const char *const name = parv[0];
	const char *const key = parv[1];

	if (! name)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (*name != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		(void) command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = mygroup_find(name)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), name);
		return;
	}

	if (! groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		(void) command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (si->su)
	{
		const char *const challenge = create_weak_challenge(si, entity(mg)->name);

		if (! challenge)
		{
			(void) command_fail(si, fault_internalerror, _("Failed to create challenge"));
			return;
		}
		if (! key)
		{
			(void) command_success_nodata(si, _("This is a friendly reminder that you are about to "
			                                    "\2DESTROY\2 the group \2%s\2."), entity(mg)->name);

			(void) command_success_nodata(si, _("To avoid accidental use of this command, this operation "
			                                    "has to be confirmed. Please confirm by replying with "
			                                    "\2/msg %s DROP %s %s\2"), si->service->disp,
			                                    entity(mg)->name, challenge);
			return;
		}
		if (strcmp(challenge, key) != 0)
		{
			(void) command_fail(si, fault_badparams, _("Invalid key for \2%s\2."), "DROP");
			return;
		}
	}

	(void) logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", entity(mg)->name);
	(void) command_success_nodata(si, _("The group \2%s\2 has been dropped."), entity(mg)->name);

	(void) remove_group_chanacs(mg);
	(void) hook_call_group_drop(mg);
	(void) atheme_object_unref(mg);
}

static void
gs_cmd_fdrop_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char *parv[])
{
	const char *const name = parv[0];

	if (! name)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	if (*name != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		(void) command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = mygroup_find(name)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), name);
		return;
	}

	(void) wallops("\2%s\2 dropped the group \2%s\2", get_oper_name(si), entity(mg)->name);
	(void) logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FDROP: \2%s\2", entity(mg)->name);
	(void) command_success_nodata(si, _("The group \2%s\2 has been dropped."), entity(mg)->name);

	(void) remove_group_chanacs(mg);
	(void) hook_call_group_drop(mg);
	(void) atheme_object_unref(mg);
}

static struct command gs_cmd_drop = {
	.name           = "DROP",
	.desc           = N_("Drops a group registration."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_drop_func,
	.help           = { .path = "groupserv/drop" },
};

static struct command gs_cmd_fdrop = {
	.name           = "FDROP",
	.desc           = N_("Force drops a group registration."),
	.access         = PRIV_GROUP_ADMIN,
	.maxparc        = 1,
	.cmd            = &gs_cmd_fdrop_func,
	.help           = { .path = "groupserv/fdrop" },
};

static void
mod_init(struct module *const restrict m)
{
	(void) use_groupserv_main_symbols(m);

	(void) service_named_bind_command("groupserv", &gs_cmd_drop);
	(void) service_named_bind_command("groupserv", &gs_cmd_fdrop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("groupserv", &gs_cmd_drop);
	(void) service_named_unbind_command("groupserv", &gs_cmd_fdrop);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
