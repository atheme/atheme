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
gs_cmd_set_email_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET EMAIL");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET EMAIL <!group> [email]"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET EMAIL");
		(void) command_fail(si, fault_badparams, _("Syntax: SET EMAIL <!group> [email]"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), group);
		return;
	}

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	const char *const param = parv[1];

	if (! param || strcasecmp(param, "NONE") == 0 || strcasecmp(param, "OFF") == 0)
	{
		if (metadata_find(mg, "email"))
		{
			(void) metadata_delete(mg, "email");
			(void) command_success_nodata(si, _("The e-mail address for group \2%s\2 was deleted."), group);
			(void) logcommand(si, CMDLOG_SET, "SET:EMAIL:NONE: \2%s\2", group);
			return;
		}

		(void) command_fail(si, fault_nochange, _("The e-mail address for group \2%s\2 was not set."), group);
		return;
	}

	if (! validemail(param))
	{
		(void) command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), param);
		return;
	}

	// we'll overwrite any existing metadata
	(void) metadata_add(mg, "email", param);

	(void) logcommand(si, CMDLOG_SET, "SET:EMAIL: \2%s\2 \2%s\2", group, param);
	(void) command_success_nodata(si, _("The e-mail address for group \2%s\2 has been set to \2%s\2."), group, param);
}

static struct command gs_cmd_set_email = {
	.name           = "EMAIL",
	.desc           = N_("Sets the group e-mail address."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_email_func,
	.help           = { .path = "groupserv/set_email" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");
	MODULE_TRY_REQUEST_SYMBOL(m, gs_set_cmdtree, "groupserv/set", "gs_set_cmdtree");

	(void) command_add(&gs_cmd_set_email, *gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&gs_cmd_set_email, *gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_email", MODULE_UNLOAD_CAPABILITY_OK)
