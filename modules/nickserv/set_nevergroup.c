/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Prevents you from being added to group access lists.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_nevergroup(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_NEVERGROUP ) == MU_NEVERGROUP;
}

// SET NEVERGROUP <ON|OFF>
static void
ns_cmd_set_nevergroup(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NEVERGROUP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVERGROUP & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NEVERGROUP:ON");

		si->smu->flags |= MU_NEVERGROUP;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVERGROUP & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NEVERGROUP:OFF");

		si->smu->flags &= ~MU_NEVERGROUP;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NEVERGROUP", entity(si->smu)->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NEVERGROUP");
		return;
	}
}

static struct command ns_set_nevergroup = {
	.name           = "NEVERGROUP",
	.desc           = N_("Prevents you from being added to group access lists."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_nevergroup,
	.help           = { .path = "nickserv/set_nevergroup" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_nevergroup, *ns_set_cmdtree);

	static struct list_param nevergroup;
	nevergroup.opttype = OPT_BOOL;
	nevergroup.is_match = has_nevergroup;

	list_register("nevergroup", &nevergroup);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_nevergroup, *ns_set_cmdtree);

	list_unregister("nevergroup");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_nevergroup", MODULE_UNLOAD_CAPABILITY_OK)
