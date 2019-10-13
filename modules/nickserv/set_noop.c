/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Prevents services from setting modes upon you automatically.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_noop(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_NOOP ) == MU_NOOP;
}

// SET NOOP [ON|OFF]
static void
ns_cmd_set_noop(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOOP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOOP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOOP:ON");

		si->smu->flags |= MU_NOOP;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOOP", entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOOP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOOP:OFF");

		si->smu->flags &= ~MU_NOOP;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOOP", entity(si->smu)->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NOOP");
		return;
	}
}

static struct command ns_set_noop = {
	.name           = "NOOP",
	.desc           = N_("Prevents services from setting modes upon you automatically."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_noop,
	.help           = { .path = "nickserv/set_noop" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_noop, *ns_set_cmdtree);

	static struct list_param noop;
	noop.opttype = OPT_BOOL;
	noop.is_match = has_noop;

	list_register("noop", &noop);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_noop, *ns_set_cmdtree);

	list_unregister("noop");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_noop", MODULE_UNLOAD_CAPABILITY_OK)
