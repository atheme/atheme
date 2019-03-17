/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

static const struct groupserv_core_symbols *gcsyms = NULL;
static mowgli_patricia_t **gs_set_cmdtree = NULL;

static void
gs_cmd_set_public_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 2)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PUBLIC");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET PUBLIC <!group> <ON|OFF>"));
		return;
	}

	const char *const group = parv[0];
	const char *const param = parv[1];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PUBLIC");
		(void) command_fail(si, fault_badparams, _("Syntax: SET PUBLIC <!group> <ON|OFF>"));
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
		if (mg->flags & MG_PUBLIC)
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is already public."), group);
			return;
		}

		mg->flags |= MG_PUBLIC;

		(void) logcommand(si, CMDLOG_SET, "SET:PUBLIC:ON: \2%s\2", group);
		(void) command_success_nodata(si, _("\2%s\2 is now public."), group);
	}
	else if (strcasecmp(param, "OFF") == 0)
	{
		if (! (mg->flags & MG_PUBLIC))
		{
			(void) command_fail(si, fault_nochange, _("\2%s\2 is not public already."), group);
			return;
		}

		mg->flags &= ~MG_PUBLIC;

		(void) logcommand(si, CMDLOG_SET, "SET:PUBLIC:OFF: \2%s\2", group);
		(void) command_success_nodata(si, _("\2%s\2 is no longer public."), group);
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PUBLIC");
		(void) command_fail(si, fault_badparams, _("Syntax: SET PUBLIC <!group> <ON|OFF>"));
	}
}

static struct command gs_cmd_set_public = {
	.name           = "PUBLIC",
	.desc           = N_("Sets the group as public."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_public_func,
	.help           = { .path = "groupserv/set_public" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");
	MODULE_TRY_REQUEST_SYMBOL(m, gs_set_cmdtree, "groupserv/set_core", "gs_set_cmdtree");

	(void) command_add(&gs_cmd_set_public, *gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&gs_cmd_set_public, *gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_public", MODULE_UNLOAD_CAPABILITY_OK)
