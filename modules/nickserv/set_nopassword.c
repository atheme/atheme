/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2015 Max Teufel <max@teufelsnetz.com>
 *
 * Allows you to opt-out of password-based authentication methods.
 */

#include <atheme.h>
#include "list_common.h"
#include "list.h"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static bool
has_nopassword(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return ( mu->flags & MU_NOPASSWORD ) == MU_NOPASSWORD;
}

// SET NOPASSWORD [ON|OFF]
static void
ns_cmd_set_nopassword(struct sourceinfo *si, int parc, char *parv[])
{
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOPASSWORD");
		return;
	}

	if (!strcasecmp("ON", setting))
	{
		if (MU_NOPASSWORD & si->smu->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "NOPASSWORD", entity(si->smu)->name);
			return;
		}

		if (si->smu->cert_fingerprints.head == NULL && metadata_find(si->smu, "private:pubkey") == NULL && metadata_find(si->smu, "pubkey") == NULL && metadata_find(si->smu, "ecdsa-nist521p-pubkey") == NULL)
		{
			command_fail(si, fault_nochange, _("You are trying to enable NOPASSWORD without any possibility to identify without a password."));
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOPASSWORD:ON");

		si->smu->flags |= MU_NOPASSWORD;

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "NOPASSWORD" ,entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", setting))
	{
		if (!(MU_NOPASSWORD & si->smu->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "NOPASSWORD", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:NOPASSWORD:OFF");

		si->smu->flags &= ~MU_NOPASSWORD;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "NOPASSWORD", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NOPASSWORD");
		return;
	}
}

static struct command ns_set_nopassword = {
	.name           = "NOPASSWORD",
	.desc           = N_("Allows you to disable any password-based authentication methods except for XMLRPC/JSONRPC."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_nopassword,
	.help           = { .path = "nickserv/set_nopassword" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	use_nslist_main_symbols(m);

	command_add(&ns_set_nopassword, *ns_set_cmdtree);

	static struct list_param nopassword;
	nopassword.opttype = OPT_BOOL;
	nopassword.is_match = has_nopassword;

	list_register("nopassword", &nopassword);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_nopassword, *ns_set_cmdtree);

	list_unregister("nopassword");
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_nopassword", MODULE_UNLOAD_CAPABILITY_OK)
