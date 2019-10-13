/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Allows you to opt-out of channel entry messages.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_nogreet(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_NOGREET ) == MU_NOGREET;
}

// SET NOGREET [ON|OFF]
static void
ns_cmd_set_nogreet(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOGREET");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOGREET & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOGREET", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOGREET:ON");

		si->smu->flags |= MU_NOGREET;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOGREET" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOGREET & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOGREET", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOGREET:OFF");

		si->smu->flags &= ~MU_NOGREET;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOGREET", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NOGREET");
		return;
	}
}

static struct command ns_set_nogreet = {
	.name           = "NOGREET",
	.desc           = N_("Allows you to opt-out of channel entry messages."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_nogreet,
	.help           = { .path = "nickserv/set_nogreet" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_nogreet, *ns_set_cmdtree);

	static struct list_param nogreet;
	nogreet.opttype = OPT_BOOL;
	nogreet.is_match = has_nogreet;

	list_register("nogreet", &nogreet);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	list_unregister("nogreet");
	command_delete(&ns_set_nogreet, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_nogreet", MODULE_UNLOAD_CAPABILITY_OK)
