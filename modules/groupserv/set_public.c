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
gs_cmd_set_public(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;

	if (!parv[0] || !parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PUBLIC");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <!group> PUBLIC <ON|OFF>"));
		return;
	}

	if ((mg = mygroup_find(parv[0])) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), parv[0]);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!strcasecmp(parv[1], "ON"))
	{
		if (mg->flags & MG_PUBLIC)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already public."), entity(mg)->name);
			return;
		}

		mg->flags |= MG_PUBLIC;

		logcommand(si, CMDLOG_SET, "PUBLIC:ON: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is now public."), entity(mg)->name);
	}
	else if (!strcasecmp(parv[1], "OFF"))
	{
		if (!(mg->flags & MG_PUBLIC))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not public already."), entity(mg)->name);
			return;
		}

		mg->flags &= ~MG_PUBLIC;

		logcommand(si, CMDLOG_SET, "PUBLIC:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is no longer public."), entity(mg)->name);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PUBLIC");
		command_fail(si, fault_badparams, _("Syntax: SET <!group> PUBLIC <ON|OFF>"));
	}
}

static struct command gs_set_public = {
	.name           = "PUBLIC",
	.desc           = N_("Sets the group as public."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_public,
	.help           = { .path = "groupserv/set_public" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_public, gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&gs_set_public, gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_public", MODULE_UNLOAD_CAPABILITY_OK)
