/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the NickServ SET ENFORCETIMEOUT function.
 */

#include <atheme.h>

static mowgli_patricia_t **ns_set_cmdtree = NULL;

static void
ns_cmd_set_enforcetime(struct sourceinfo *si, int parc, char *parv[])
{
	unsigned int enforcetime;
	char *setting = parv[0];

	if (! parc)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET ENFORCETIME");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCETIME TIME|DEFAULT"));
		return;
	}

	if (! metadata_find(si->smu, "private:doenforce"))
	{
		command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for account \2%s\2."), "ENFORCE", entity(si->smu)->name);
		return;
	}

	if (strcasecmp(setting, "DEFAULT") == 0)
	{
		logcommand(si, CMDLOG_SET, "SET:ENFORCETIME:DEFAULT");
		metadata_delete(si->smu, "private:enforcetime");
		command_success_nodata(si, _("The \2%s\2 for account \2%s\2 has been reset to default, which is \2%u\2 seconds."), "ENFORCETIME", entity(si->smu)->name, nicksvs.enforce_delay);
	}
	else if (! string_to_uint(setting, &enforcetime) || ! enforcetime || enforcetime > 180)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET ENFORCETIME");
	}
	else
	{
		logcommand(si, CMDLOG_SET, "SET:ENFORCETIME: %u", enforcetime);
		metadata_add(si->smu, "private:enforcetime", setting);
		command_success_nodata(si, _("The \2%s\2 for account \2%s\2 has been set to \2%u\2 seconds."), "ENFORCETIME", entity(si->smu)->name, enforcetime);
	}
}

static struct command ns_set_enforcetime = {
	.name           = "ENFORCETIME",
	.desc           = N_("Configure amount of time it takes before nickname protection occurs."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_enforcetime,
	.help           = { .path = "nickserv/set_enforcetime" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")

	command_add(&ns_set_enforcetime, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&ns_set_enforcetime, *ns_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/set_enforcetime", MODULE_UNLOAD_CAPABILITY_OK)
