/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Allows you to opt-out of channel change messages.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_quietchg(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_QUIETCHG ) == MU_QUIETCHG;
}

// SET QUIETCHG [ON|OFF]
static void
ns_cmd_set_quietchg(struct sourceinfo *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET QUIETCHG");
		command_fail(si, fault_needmoreparams, _("Syntax: SET QUIETCHG <ON|OFF>"));
		return;
	}

	if (!strcasecmp("ON", setting))
	{
		if (MU_QUIETCHG & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "QUIETCHG", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:QUIETCHG:ON");

		si->smu->flags |= MU_QUIETCHG;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "QUIETCHG" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", setting))
	{
		if (!(MU_QUIETCHG & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "QUIETCHG", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:QUIETCHG:OFF");

		si->smu->flags &= ~MU_QUIETCHG;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "QUIETCHG", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET QUIETCHG");
		command_fail(si, fault_needmoreparams, _("Syntax: SET QUIETCHG <ON|OFF>"));
		return;
	}
}

static struct command ns_set_quietchg = {
	.name           = "QUIETCHG",
	.desc           = N_("Allows you to opt-out of channel change messages."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_quietchg,
	.help           = { .path = "nickserv/set_quietchg" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_quietchg, *ns_set_cmdtree);

	static struct list_param quietchg;
	quietchg.opttype = OPT_BOOL;
	quietchg.is_match = has_quietchg;

	list_register("quietchg", &quietchg);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_quietchg, *ns_set_cmdtree);

	list_unregister("quietchg");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_quietchg", MODULE_UNLOAD_CAPABILITY_OK)
