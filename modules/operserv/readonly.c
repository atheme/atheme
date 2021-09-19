/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * Controls READONLY setting.
 */

#include <atheme.h>

static void
os_cmd_readonly(struct sourceinfo *si, int parc, char *parv[])
{
	struct service *svs;
	char *action = parv[0];

	if (!action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "READONLY");
		command_fail(si, fault_needmoreparams, _("Usage: READONLY <ON|OFF>"));
		return;
	}

	if (! db_save)
	{
		(void) command_fail(si, fault_unimplemented, _("You have no write-capable database backend loaded."));
		return;
	}

	svs = service_find("operserv");

	if (!strcasecmp(action, "ON"))
	{
		if (readonly)
		{
			command_fail(si, fault_badparams, _("Read-only mode is already enabled."));
			return;
		}

		readonly = true;

		notice_global_sts(svs->me, "*", "Services are now running in readonly mode.  Any changes you make will not be saved.");

		wallops("\2%s\2 set the READONLY option.", get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "READONLY:ON");
		command_success_nodata(si, _("Read-only mode is now enabled."));

		if (commit_interval_timer)
		{
			(void) mowgli_timer_destroy(base_eventloop, commit_interval_timer);

			commit_interval_timer = NULL;
		}
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!readonly)
		{
			command_fail(si, fault_badparams, _("Read-only mode is already disabled."));
			return;
		}

		readonly = false;

		notice_global_sts(svs->me, "*", "Services are no longer running in readonly mode.  Any changes you make will now be saved.");

		wallops("\2%s\2 unset the READONLY option.", get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "READONLY:OFF");
		command_success_nodata(si, _("Read-only mode is now disabled."));

		if (commit_interval_timer)
			(void) mowgli_timer_destroy(base_eventloop, commit_interval_timer);

		commit_interval_timer = mowgli_timer_add(base_eventloop, "db_save_periodic", &db_save_periodic, NULL,
		                                         config_options.commit_interval);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "READONLY");
		command_fail(si, fault_needmoreparams, _("Usage: READONLY <ON|OFF>"));
	}
}

static struct command os_readonly = {
	.name           = "READONLY",
	.desc           = N_("Changes the state of read-only mode for services."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_readonly,
	.help           = { .path = "oservice/readonly" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	service_named_bind_command("operserv", &os_readonly);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_readonly);
}

SIMPLE_DECLARE_MODULE_V1("operserv/readonly", MODULE_UNLOAD_CAPABILITY_OK)
