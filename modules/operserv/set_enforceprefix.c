/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2011 William Pitcock, et al.
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the OperServ SET command.
 */

#include <atheme.h>

static mowgli_patricia_t **os_set_cmdtree = NULL;

static void
os_cmd_set_enforceprefix_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET ENFORCEPREFIX");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCEPREFIX <prefix>"));
		return;
	}

	const char *const param = parv[0];

	if (! *param || ! is_valid_nick(param))
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET ENFORCEPREFIX");
		(void) command_fail(si, fault_badparams, _("Syntax: SET ENFORCEPREFIX <prefix>"));
		return;
	}

	(void) sfree(nicksvs.enforce_prefix);

	nicksvs.enforce_prefix = sstrdup(param);

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%s\2."), "ENFORCEPREFIX", param);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:ENFORCEPREFIX: \2%s\2", param);
}

static struct command os_cmd_set_enforceprefix = {
	.name           = "ENFORCEPREFIX",
	.desc           = N_("Changes the prefix to use when changing the user's nick on enforcement."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_enforceprefix_func,
	.help           = { .path = "oservice/set_enforceprefix" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, os_set_cmdtree, "operserv/set_core", "os_set_cmdtree")

	(void) command_add(&os_cmd_set_enforceprefix, *os_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&os_cmd_set_enforceprefix, *os_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("operserv/set_enforceprefix", MODULE_UNLOAD_CAPABILITY_OK)
