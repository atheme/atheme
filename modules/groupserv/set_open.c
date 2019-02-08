/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static const struct groupserv_core_symbols *gcsyms = NULL;
static mowgli_patricia_t **gs_set_cmdtree = NULL;

static void
gs_cmd_set_open_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET OPEN");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET OPEN <!group> <ON|OFF>"));
		return;
	}

	const char *const group = parv[0];
	const char *const param = parv[1];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET OPEN");
		(void) command_fail(si, fault_badparams, _("Syntax: SET OPEN <!group> <ON|OFF>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_FOUNDER))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	if (strcasecmp(param, "ON") == 0)
	{
		if (! gcsyms->config->enable_open_groups)
		{
			(void) command_fail(si, fault_nochange, _("Setting groups as open has been administratively "
			                                          "disabled."));
			return;
		}

		if (mg->flags & MG_OPEN)
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is already open to anyone joining."), group);
			return;
		}

		mg->flags |= MG_OPEN;

		(void) logcommand(si, CMDLOG_SET, "SET:OPEN:ON: \2%s\2", group);
		(void) command_success_nodata(si, _("\2%s\2 is now open to anyone joining."), group);
	}
	else if (strcasecmp(param, "OFF") == 0)
	{
		if (! (mg->flags & MG_OPEN))
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is already not open to anyone joining."),
			                    group);
			return;
		}

		mg->flags &= ~MG_OPEN;

		(void) logcommand(si, CMDLOG_SET, "SET:OPEN:OFF: \2%s\2", group);
		(void) command_success_nodata(si, _("\2%s\2 is no longer open to anyone joining."), group);
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET OPEN");
		(void) command_fail(si, fault_badparams, _("Syntax: SET OPEN <!group> <ON|OFF>"));
	}
}

static struct command gs_cmd_set_open = {
	.name           = "OPEN",
	.desc           = N_("Sets the group as open for anyone to join."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_open_func,
	.help           = { .path = "groupserv/set_open" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");
	MODULE_TRY_REQUEST_SYMBOL(m, gs_set_cmdtree, "groupserv/set", "gs_set_cmdtree");

	(void) command_add(&gs_cmd_set_open, *gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&gs_cmd_set_open, *gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_open", MODULE_UNLOAD_CAPABILITY_OK)
