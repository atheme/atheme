/*
 * Copyright (c) 2005-2010 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls READONLY setting.
 */

#include "atheme.h"

static void os_cmd_readonly(struct sourceinfo *si, int parc, char *parv[]);

static struct command os_readonly = { "READONLY", N_("Changes the state of read-only mode for services."),
		      PRIV_ADMIN, 1, os_cmd_readonly, { .path = "oservice/readonly" } };

static void
mod_init(struct module *const restrict m)
{
	service_named_bind_command("operserv", &os_readonly);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("operserv", &os_readonly);
}

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

		wallops("%s set the READONLY option.", get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "READONLY:ON");
		command_success_nodata(si, _("Read-only mode is now enabled."));
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

		wallops("%s unset the READONLY option.", get_oper_name(si));
		logcommand(si, CMDLOG_ADMIN, "READONLY:OFF");
		command_success_nodata(si, _("Read-only mode is now disabled."));
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "READONLY");
		command_fail(si, fault_needmoreparams, _("Usage: READONLY <ON|OFF>"));
	}
}

SIMPLE_DECLARE_MODULE_V1("operserv/readonly", MODULE_UNLOAD_CAPABILITY_OK)
