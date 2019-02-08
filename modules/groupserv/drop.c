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
gs_cmd_drop_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		(void) command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), group);
		return;
	}

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (si->su)
	{
		const char *const challenge = create_weak_challenge(si, group);
		const char *const param = parv[1];

		if (! challenge)
		{
			(void) command_fail(si, fault_internalerror, _("Failed to create challenge"));
			return;
		}

		if (! param)
		{
			char fullcmd[BUFSIZE];

			(void) snprintf(fullcmd, sizeof fullcmd, "/%s%s DROP %s %s", (ircd->uses_rcommand == false) ?
			                "msg " : "", si->service->disp, group, challenge);

			(void) command_success_nodata(si, _("To avoid accidental use of this command, this operation "
			                                    "has to be confirmed. Please confirm by replying with "
			                                    "\2%s\2"), fullcmd);
			return;
		}

		if (strcmp(challenge, param) != 0)
		{
			(void) command_fail(si, fault_badparams, _("Invalid key for \2%s\2."), "DROP");
			return;
		}
	}

	(void) logcommand(si, CMDLOG_REGISTER, "DROP: \2%s\2", group);
	(void) command_success_nodata(si, _("The group \2%s\2 has been dropped."), group);

	(void) gcsyms->remove_group_chanacs(mg);
	(void) hook_call_group_drop(mg);
	(void) atheme_object_unref(mg);
}

static void
gs_cmd_fdrop_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DROP");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: DROP <!group>"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DROP");
		(void) command_fail(si, fault_badparams, _("Syntax: DROP <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), group);
		return;
	}

	(void) wallops("%s dropped the group \2%s\2", get_oper_name(si), group);
	(void) logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FDROP: \2%s\2", group);
	(void) command_success_nodata(si, _("The group \2%s\2 has been dropped."), group);

	(void) gcsyms->remove_group_chanacs(mg);
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
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_drop);
	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_fdrop);

	(void) hook_add_event("group_drop");
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_drop);
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_fdrop);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/drop", MODULE_UNLOAD_CAPABILITY_OK)
