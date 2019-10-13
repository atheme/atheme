/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2007 Jilles Tjoelker
 *
 * Forwards incoming memos to your e-mail address.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_emailmemos(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_EMAILMEMOS ) == MU_EMAILMEMOS;
}

// SET EMAILMEMOS [ON|OFF]
static void
ns_cmd_set_emailmemos(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET EMAILMEMOS");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (me.mta == NULL)
		{
			command_fail(si, fault_emailfail, _("Sending email is administratively disabled."));
			return;
		}
		if (MU_EMAILMEMOS & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "EMAILMEMOS", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:EMAILMEMOS:ON");
		si->smu->flags |= MU_EMAILMEMOS;
		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "EMAILMEMOS", entity(si->smu)->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_EMAILMEMOS & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "EMAILMEMOS", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:EMAILMEMOS:OFF");
		si->smu->flags &= ~MU_EMAILMEMOS;
		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "EMAILMEMOS", entity(si->smu)->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET EMAILMEMOS");
		return;
	}
}

static struct command ns_set_emailmemos = {
	.name           = "EMAILMEMOS",
	.desc           = N_("Forwards incoming memos to your e-mail address."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_emailmemos,
	.help           = { .path = "nickserv/set_emailmemos" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_emailmemos, *ns_set_cmdtree);

	static struct list_param emailmemos;
	emailmemos.opttype = OPT_BOOL;
	emailmemos.is_match = has_emailmemos;

	list_register("emailmemos", &emailmemos);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_emailmemos, *ns_set_cmdtree);
	list_unregister("emailmemos");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_emailmemos", MODULE_UNLOAD_CAPABILITY_OK)
