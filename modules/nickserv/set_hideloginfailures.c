/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

#define METADATA_KEY_NAME "private:showloginfailures:optout"

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static void
ns_cmd_set_hideloginfailures_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (! parc)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET HIDELOGINFAILURES");
	}
	else if (strcasecmp("ON", parv[0]) == 0)
	{
		if (metadata_find(si->smu, METADATA_KEY_NAME))
		{
			(void) command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for account \2%s\2."),
			                                          "HIDELOGINFAILURES", entity(si->smu)->name);
			return;
		}

		(void) metadata_add(si->smu, METADATA_KEY_NAME, "ON");
		(void) logcommand(si, CMDLOG_SET, "SET:HIDELOGINFAILURES:ON");
		(void) command_success_nodata(si, _("The \2%s\2 flag has been set for account \2%s\2."),
		                                    "HIDELOGINFAILURES", entity(si->smu)->name);
	}
	else if (strcasecmp("OFF", parv[0]) == 0)
	{
		if (! metadata_find(si->smu, METADATA_KEY_NAME))
		{
			(void) command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."),
			                                          "HIDELOGINFAILURES", entity(si->smu)->name);
			return;
		}

		(void) metadata_delete(si->smu, METADATA_KEY_NAME);
		(void) logcommand(si, CMDLOG_SET, "SET:HIDELOGINFAILURES:OFF");
		(void) command_success_nodata(si, _("The \2%s\2 flag has been removed for account \2%s\2."),
		                                    "HIDELOGINFAILURES", entity(si->smu)->name);
	}
	else
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET HIDELOGINFAILURES");
	}
}

static struct command ns_cmd_set_hideloginfailures = {
	.name           = "HIDELOGINFAILURES",
	.desc           = N_("Opts you out of Login Failure notices for your account."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_hideloginfailures_func,
	.help           = { .path = "nickserv/set_hideloginfailures" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree");

	(void) command_add(&ns_cmd_set_hideloginfailures, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&ns_cmd_set_hideloginfailures, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_hideloginfailures", MODULE_UNLOAD_CAPABILITY_OK)
