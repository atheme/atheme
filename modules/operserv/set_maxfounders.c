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
os_cmd_set_maxfounders_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET MAXFOUNDERS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET MAXFOUNDERS <value>"));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	/* Yes, I know how arbitrary the high value is, this is what it is in confprocess.c
	 * (I rounded it down though) -- JD
	 */
	if (! string_to_uint(param, &value) || ! value || value > 41)
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET MAXFOUNDERS");
		(void) command_fail(si, fault_badparams, _("Syntax: SET MAXFOUNDERS <value>"));
		return;
	}

	chansvs.maxfounders = value;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2."), "MAXFOUNDERS", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:MAXFOUNDERS: \2%u\2", value);
}

static struct command os_cmd_set_maxfounders = {
	.name           = "MAXFOUNDERS",
	.desc           = N_("Sets the maximum number of founders per channel."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_maxfounders_func,
	.help           = { .path = "oservice/set_maxfounders" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, os_set_cmdtree, "operserv/set_core", "os_set_cmdtree")

	(void) command_add(&os_cmd_set_maxfounders, *os_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&os_cmd_set_maxfounders, *os_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("operserv/set_maxfounders", MODULE_UNLOAD_CAPABILITY_OK)
