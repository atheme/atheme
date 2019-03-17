/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_regnolimit_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGNOLIMIT");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
		return;
	}

	const char *const group = parv[0];
	const char *const param = parv[1];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGNOLIMIT");
		(void) command_fail(si, fault_badparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	if (strcasecmp(param, "ON") == 0)
	{
		if (mg->flags & MG_REGNOLIMIT)
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 can already bypass registration limits."),
			                    group);
			return;
		}

		mg->flags |= MG_REGNOLIMIT;

		(void) wallops("%s set the REGNOLIMIT option on the group \2%s\2.", get_oper_name(si), group);
		(void) logcommand(si, CMDLOG_ADMIN, "SET:REGNOLIMIT:ON: \2%s\2", group);
		(void) command_success_nodata(si, _("\2%s\2 can now bypass registration limits."), group);
	}
	else if (strcasecmp(param, "OFF") == 0)
	{
		if (! (mg->flags & MG_REGNOLIMIT))
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 cannot bypass registration limits."),
			                    group);
			return;
		}

		mg->flags &= ~MG_REGNOLIMIT;

		(void) wallops("%s removed the REGNOLIMIT option from the group \2%s\2.", get_oper_name(si), group);
		(void) logcommand(si, CMDLOG_ADMIN, "SET:REGNOLIMIT:OFF: \2%s\2", group);
		(void) command_success_nodata(si, _("\2%s\2 cannot bypass registration limits anymore."), group);
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGNOLIMIT");
		(void) command_fail(si, fault_badparams, _("Syntax: REGNOLIMIT <!group> <ON|OFF>"));
	}
}

static struct command gs_cmd_regnolimit = {
	.name           = "REGNOLIMIT",
	.desc           = N_("Allow a group to bypass registration limits."),
	.access         = PRIV_GROUP_ADMIN,
	.maxparc        = 2,
	.cmd            = &gs_cmd_regnolimit_func,
	.help           = { .path = "groupserv/regnolimit" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_regnolimit);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_regnolimit);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/regnolimit", MODULE_UNLOAD_CAPABILITY_OK)
