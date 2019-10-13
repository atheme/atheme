/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Hides your e-mail address.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_hidemail(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_HIDEMAIL ) == MU_HIDEMAIL;
}

// SET HIDEMAIL [ON|OFF]
static void
ns_cmd_set_hidemail(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET HIDEMAIL");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "HIDEMAIL", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:HIDEMAIL:ON");

		si->smu->flags |= MU_HIDEMAIL;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "HIDEMAIL" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "HIDEMAIL", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:HIDEMAIL:OFF");

		si->smu->flags &= ~MU_HIDEMAIL;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "HIDEMAIL", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET HIDEMAIL");
		return;
	}
}

static struct command ns_set_hidemail = {
	.name           = "HIDEMAIL",
	.desc           = N_("Hides your e-mail address."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_hidemail,
	.help           = { .path = "nickserv/set_hidemail" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_hidemail, *ns_set_cmdtree);

	static struct list_param hidemail;
	hidemail.opttype = OPT_BOOL;
	hidemail.is_match = has_hidemail;

	list_register("hidemail", &hidemail);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_hidemail, *ns_set_cmdtree);
	list_unregister("hidemail");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_hidemail", MODULE_UNLOAD_CAPABILITY_OK)
