/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2014 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET PUBACL command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_pubacl(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PUBACL");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, parv[0]);
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_PUBACL & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "PUBACL", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:PUBACL:ON: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 enabled the PUBACL flag", get_source_name(si));

 		mc->flags |= MC_PUBACL;

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "PUBACL", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_PUBACL & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "PUBACL", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:PUBACL:OFF: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 disabled the PUBACL flag", get_source_name(si));

		mc->flags &= ~MC_PUBACL;

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "PUBACL", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PUBACL");
		return;
	}
}

static struct command cs_set_pubacl = {
	.name           = "PUBACL",
	.desc           = N_("Allows the channel ACL to be public."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_pubacl,
	.help           = { .path = "cservice/set_pubacl" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_pubacl, *cs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_pubacl, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_pubacl", MODULE_UNLOAD_CAPABILITY_OK)
