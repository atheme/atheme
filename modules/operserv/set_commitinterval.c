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
os_cmd_set_commitinterval_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET COMMITINTERVAL");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET COMMITINTERVAL <minutes>"));
		return;
	}

	if (! db_save)
	{
		(void) command_fail(si, fault_unimplemented, _("You have no write-capable database backend loaded."));
		return;
	}

	const char *const param = parv[0];
	unsigned int value;

	if (! string_to_uint(param, &value))
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET COMMITINTERVAL");
		(void) command_fail(si, fault_badparams, _("Syntax: SET COMMITINTERVAL <minutes>"));
		return;
	}
	if (value < 1U || value > 60U)
	{
		(void) command_fail(si, fault_badparams, _("The commit interval must be between 1 and 60 minutes."));
		return;
	}

	config_options.commit_interval = value * SECONDS_PER_MINUTE;

	(void) command_success_nodata(si, _("You have successfully set \2%s\2 to \2%u\2 minutes."), "COMMITINTERVAL", value);
	(void) logcommand(si, CMDLOG_ADMIN, "SET:COMMITINTERVAL: \2%u\2", value);

	if (readonly)
	{
		(void) command_success_nodata(si, _("Services is running read-only; this will have no effect."));
		return;
	}

	if (commit_interval_timer)
		(void) mowgli_timer_destroy(base_eventloop, commit_interval_timer);

	commit_interval_timer = mowgli_timer_add(base_eventloop, "db_save_periodic", &db_save_periodic, NULL,
	                                         config_options.commit_interval);
}

static struct command os_cmd_set_commitinterval = {
	.name           = "COMMITINTERVAL",
	.desc           = N_("Changes how often the database is written to disk."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_set_commitinterval_func,
	.help           = { .path = "oservice/set_commitinterval" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, os_set_cmdtree, "operserv/set_core", "os_set_cmdtree")

	(void) command_add(&os_cmd_set_commitinterval, *os_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&os_cmd_set_commitinterval, *os_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("operserv/set_commitinterval", MODULE_UNLOAD_CAPABILITY_OK)
