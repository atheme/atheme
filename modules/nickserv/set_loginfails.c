/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2022 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains routines to handle the NickServ SET LOGINFAILS command.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_loginfails(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_LOGINFAILS ) == MU_LOGINFAILS;
}

// SET LOGINFAILS ON|OFF
static void
ns_cmd_set_loginfails(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET LOGINFAILS");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_LOGINFAILS & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for \2%s\2."), "LOGINFAILS", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:LOGINFAILS:ON");

		si->smu->flags |= MU_LOGINFAILS;

		command_success_nodata(si, _("The \2%s\2 flag has been set for \2%s\2."), "LOGINFAILS" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_LOGINFAILS & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for \2%s\2."), "LOGINFAILS", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:LOGINFAILS:OFF");

		si->smu->flags &= ~MU_LOGINFAILS;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for \2%s\2."), "LOGINFAILS", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET LOGINFAILS");
		return;
	}
}

static struct command ns_set_loginfails = {
	.name           = "LOGINFAILS",
	.desc           = N_("Tell you when someone fails to login as you."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_loginfails,
	.help           = { .path = "nickserv/set_loginfails" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_loginfails, *ns_set_cmdtree);

	static struct list_param loginfails;
	loginfails.opttype = OPT_BOOL;
	loginfails.is_match = has_loginfails;

	list_register("loginfails", &loginfails);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_loginfails, *ns_set_cmdtree);

	list_unregister("loginfails");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_loginfails", MODULE_UNLOAD_CAPABILITY_OK)
