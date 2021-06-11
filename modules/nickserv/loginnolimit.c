/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2021 Atheme Project (http://atheme.org/)
 *
 * Controls LOGINNOLIMIT setting.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static bool
has_loginnolimit(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_LOGINNOLIMIT ) == MU_LOGINNOLIMIT;
}

static void
ns_cmd_loginnolimit(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	struct myuser *mu;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LOGINNOLIMIT");
		command_fail(si, fault_needmoreparams, _("Usage: LOGINNOLIMIT <account> <ON|OFF>"));
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (mu->flags & MU_LOGINNOLIMIT)
		{
			command_fail(si, fault_badparams, _("\2%s\2 can already bypass login limits."), entity(mu)->name);
			return;
		}

		mu->flags |= MU_LOGINNOLIMIT;

		wallops("\2%s\2 set the LOGINNOLIMIT option for the account \2%s\2.", get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "LOGINNOLIMIT:ON: \2%s\2", entity(mu)->name);
		command_success_nodata(si, _("\2%s\2 can now bypass login limits."), entity(mu)->name);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mu->flags & MU_LOGINNOLIMIT))
		{
			command_fail(si, fault_badparams, _("\2%s\2 cannot bypass login limits."), entity(mu)->name);
			return;
		}

		mu->flags &= ~MU_LOGINNOLIMIT;

		wallops("\2%s\2 removed the LOGINNOLIMIT option on the account \2%s\2.", get_oper_name(si), entity(mu)->name);
		logcommand(si, CMDLOG_ADMIN, "LOGINNOLIMIT:OFF: \2%s\2", entity(mu)->name);
		command_success_nodata(si, _("\2%s\2 cannot bypass login limits anymore."), entity(mu)->name);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "LOGINNOLIMIT");
		command_fail(si, fault_needmoreparams, _("Usage: LOGINNOLIMIT <account> <ON|OFF>"));
	}
}

static struct command ns_loginnolimit = {
	.name           = "LOGINNOLIMIT",
	.desc           = N_("Allow a user to bypass login limits."),
	.access         = PRIV_ADMIN,
	.maxparc        = 2,
	.cmd            = &ns_cmd_loginnolimit,
	.help           = { .path = "nickserv/loginnolimit" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	use_nslist_main_symbols(m);

	service_named_bind_command("nickserv", &ns_loginnolimit);

	static struct list_param loginnolimit;
	loginnolimit.opttype = OPT_BOOL;
	loginnolimit.is_match = has_loginnolimit;

	list_register("loginnolimit", &loginnolimit);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("nickserv", &ns_loginnolimit);

	list_unregister("loginnolimit");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/loginnolimit", MODULE_UNLOAD_CAPABILITY_OK)
