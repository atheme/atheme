/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET VERBOSE command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_verbose(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET VERBOSE");
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

	if (!strcasecmp("ON", parv[1]) || !strcasecmp("ALL", parv[1]))
	{
		if (MC_VERBOSE & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "VERBOSE", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:VERBOSE:ON: \2%s\2", mc->name);

 		mc->flags &= ~MC_VERBOSE_OPS;
 		mc->flags |= MC_VERBOSE;

		verbose(mc, "\2%s\2 enabled the VERBOSE flag", get_source_name(si));
		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "VERBOSE", mc->name);
		return;
	}

	else if (!strcasecmp("OPS", parv[1]))
	{
		if (MC_VERBOSE_OPS & mc->flags)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "VERBOSE_OPS", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:VERBOSE:OPS: \2%s\2", mc->name);

		if (mc->flags & MC_VERBOSE)
		{
			verbose(mc, "\2%s\2 restricted VERBOSE to chanops", get_source_name(si));
 			mc->flags &= ~MC_VERBOSE;
 			mc->flags |= MC_VERBOSE_OPS;
		}
		else
		{
 			mc->flags |= MC_VERBOSE_OPS;
			verbose(mc, "\2%s\2 enabled the VERBOSE_OPS flag", get_source_name(si));
		}

		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "VERBOSE_OPS", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!((MC_VERBOSE | MC_VERBOSE_OPS) & mc->flags))
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "VERBOSE", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:VERBOSE:OFF: \2%s\2", mc->name);

		if (mc->flags & MC_VERBOSE)
			verbose(mc, "\2%s\2 disabled the VERBOSE flag", get_source_name(si));
		else
			verbose(mc, "\2%s\2 disabled the VERBOSE_OPS flag", get_source_name(si));
		mc->flags &= ~(MC_VERBOSE | MC_VERBOSE_OPS);

		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "VERBOSE", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET VERBOSE");
		return;
	}
}

static struct command cs_set_verbose = {
	.name           = "VERBOSE",
	.desc           = N_("Notifies channel about access list modifications."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_verbose,
	.help           = { .path = "cservice/set_verbose" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_verbose, *cs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_verbose, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_verbose", MODULE_UNLOAD_CAPABILITY_OK)
