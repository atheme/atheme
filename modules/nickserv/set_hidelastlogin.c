/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2016 Austin Ellis <siniStar -at- IRC4Fun.net>
 *
 * Hides (opts you OUT) of the automatic Last Login notice upon
 * successfully authenticating to your account.
 */

#include <atheme.h>

static mowgli_patricia_t **ns_set_cmdtree = NULL;

// SET HIDELASTLOGIN [ON|OFF]
static void
ns_cmd_set_hidelastlogin(struct sourceinfo *si, int parc, char *parv[])
{
	char *params = parv[0];


	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET HIDELASTLOGIN");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (metadata_find(si->smu, "private:showlast:optout"))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:HIDELASTLOGIN:ON");

		metadata_add(si->smu, "private:showlast:optout", "ON");

		command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!metadata_find(si->smu, "private:showlast:optout"))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:HIDELASTLOGIN:OFF");

		metadata_delete(si->smu, "private:showlast:optout");

		command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."), "HIDELASTLOGIN", entity(si->smu)->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET HIDELASTLOGIN");
		return;
	}
}

static struct command ns_set_hidelastlogin = {
	.name           = "HIDELASTLOGIN",
	.desc           = N_("Opts you out of Last Login notices upon identifying to your account."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_hidelastlogin,
	.help           = { .path = "nickserv/set_hidelastlogin" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	command_add(&ns_set_hidelastlogin, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_hidelastlogin, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_hidelastlogin", MODULE_UNLOAD_CAPABILITY_OK)
