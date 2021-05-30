/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2006-2009 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the CService SET GAMESERV command.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_set_cmdtree = NULL;

static void
cs_cmd_set_gameserv(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	const char *val;
	struct metadata *md;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET GAMESERV");
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

	if (!strcasecmp("ALL", parv[1]))
		val = "ALL";
	else if (!strcasecmp("VOICE", parv[1]) || !strcasecmp("VOICES", parv[1]))
		val = "VOICE";
	else if (!strcasecmp("OP", parv[1]) || !strcasecmp("OPS", parv[1]))
		val = "OP";
	else if (!strcasecmp("OFF", parv[1]))
		val = NULL;
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET GAMESERV");
		return;
	}

	md = metadata_find(mc, "gameserv");

	if (val != NULL)
	{
		if (md != NULL && !strcasecmp(md->value, val))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already set to %s for \2%s\2."), "GAMESERV", val, mc->name);
			return;
		}
		logcommand(si, CMDLOG_SET, "SET:GAMESERV: \2%s\2 to \2%s\2", mc->name, val);
		verbose(mc, "\2%s\2 set the GAMESERV flag to \2%s\2", get_source_name(si), val);

		metadata_add(mc, "gameserv", val);

		command_success_nodata(si, _("\2%s\2 has been set to \2%s\2 for \2%s\2."), "GAMESERV", val, mc->name);

		return;
	}
	else
	{
		if (md == NULL)
		{
			command_fail(si, fault_nochange, _("\2%s\2 was not set for \2%s\2."), "GAMESERV", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET:GAMESERV:OFF: \2%s\2", mc->name);
		verbose(mc, "\2%s\2 disabled the GAMESERV flag", get_source_name(si));

		metadata_delete(mc, "gameserv");

		command_success_nodata(si, _("\2%s\2 has been disabled for \2%s\2."), "GAMESERV", mc->name);
	}
}

static struct command cs_set_gameserv = {
	.name           = "GAMESERV",
	.desc           = N_("Allows or disallows gaming services."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_set_gameserv,
	.help           = { .path = "cservice/set_gameserv" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree")

	command_add(&cs_set_gameserv, *cs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_set_gameserv, *cs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_gameserv", MODULE_UNLOAD_CAPABILITY_OK)
