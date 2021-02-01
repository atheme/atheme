/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Controls noexpire options for channels.
 */

#include <atheme.h>

static void
cs_cmd_hold(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	struct mychan *mc;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HOLD");
		command_fail(si, fault_needmoreparams, _("Usage: HOLD <#channel> <ON|OFF>"));
		return;
	}

	if (*target != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HOLD");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (mc->flags & MC_HOLD)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already held."), target);
			return;
		}

		mc->flags |= MC_HOLD;

		wallops("\2%s\2 set the HOLD option for the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "HOLD:ON: \2%s\2", mc->name);
		command_success_nodata(si, _("\2%s\2 is now held."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mc->flags & MC_HOLD))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not held."), target);
			return;
		}

		mc->flags &= ~MC_HOLD;

		wallops("\2%s\2 removed the HOLD option on the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "HOLD:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("\2%s\2 is no longer held."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HOLD");
		command_fail(si, fault_badparams, _("Usage: HOLD <#channel> <ON|OFF>"));
	}
}

static struct command cs_hold = {
	.name           = "HOLD",
	.desc           = N_("Prevents a channel from expiring."),
	.access         = PRIV_HOLD,
	.maxparc        = 2,
	.cmd            = &cs_cmd_hold,
	.help           = { .path = "cservice/hold" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_hold);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_hold);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/hold", MODULE_UNLOAD_CAPABILITY_OK)
