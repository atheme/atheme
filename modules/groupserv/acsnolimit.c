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
gs_cmd_acsnolimit(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACSNOLIMIT");
		command_fail(si, fault_needmoreparams, _("Syntax: ACSNOLIMIT <!group> <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}

	if (!strcasecmp(parv[1], "ON"))
	{
		if (mg->flags & MG_ACSNOLIMIT)
		{
			command_fail(si, fault_nochange, _("\2%s\2 can already bypass access list limits."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_ACSNOLIMIT;

		wallops("\2%s\2 set the ACSNOLIMIT option on the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "ACSNOLIMIT:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 can now bypass access list limits."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_ACSNOLIMIT))
		{
			command_fail(si, fault_nochange, _("\2%s\2 cannot bypass access list limits."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_ACSNOLIMIT;

		wallops("\2%s\2 removed the ACSNOLIMIT option from the group \2%s\2.", get_oper_name(si), entity(mg)->name);
		logcommand(si, CMDLOG_ADMIN, "ACSNOLIMIT:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 cannot bypass access list limits anymore."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ACSNOLIMIT");
		command_fail(si, fault_badparams, _("Syntax: ACSNOLIMIT <!group> <ON|OFF>"));
	}
}

static struct command gs_acsnolimit = {
	.name           = "ACSNOLIMIT",
	.desc           = N_("Allow a group to bypass access list limits."),
	.access         = PRIV_GROUP_ADMIN,
	.maxparc        = 2,
	.cmd            = &gs_cmd_acsnolimit,
	.help           = { .path = "groupserv/acsnolimit" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_acsnolimit);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_acsnolimit);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/acsnolimit", MODULE_UNLOAD_CAPABILITY_OK)
