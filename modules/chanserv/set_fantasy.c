/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET FANTASY command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_fantasy(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET FANTASY");
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
		struct metadata *md = metadata_find(mc, "disable_fantasy");

		if (!md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "FANTASY", mc->name);
			return;
		}

		metadata_delete(mc, "disable_fantasy");

		logcommand(si, CMDLOG_SET, "SET:FANTASY:ON: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 enabled the FANTASY flag", get_source_name(si));
		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "FANTASY", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		struct metadata *md = metadata_find(mc, "disable_fantasy");

		if (md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "FANTASY", mc->name);
			return;
		}

		metadata_add(mc, "disable_fantasy", "on");

		logcommand(si, CMDLOG_SET, "SET:FANTASY:OFF: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 disabled the FANTASY flag", get_source_name(si));
		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "FANTASY", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET FANTASY");
		return;
	}
}

static struct command cs_set_fantasy = {
	.name           = "FANTASY",
	.desc           = N_("Allows or disallows in-channel commands."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_fantasy,
	.help           = { .path = "cservice/set_fantasy" },
};

static void
cs_set_fantasy_config_ready(void *unused)
{
	if (chansvs.fantasy)
		cs_set_fantasy.access = AC_NONE;
	else
		cs_set_fantasy.access = AC_DISABLED;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_fantasy, *cs_set_cmdtree);

	hook_add_config_ready(cs_set_fantasy_config_ready);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_fantasy, *cs_set_cmdtree);

	hook_del_config_ready(cs_set_fantasy_config_ready);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_fantasy", MODULE_UNLOAD_CAPABILITY_OK)
