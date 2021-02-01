/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock
 *
 * Marking for channels.
 */

#include <atheme.h>

static void
cs_cmd_mark(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	struct mychan *mc;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("Usage: MARK <#channel> <ON|OFF> [note]"));
		return;
	}

	if (target[0] != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <#channel> ON <note>"));
			return;
		}

		if (metadata_find(mc, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already marked."), target);
			return;
		}

		metadata_add(mc, "private:mark:setter", get_oper_name(si));
		metadata_add(mc, "private:mark:reason", info);
		metadata_add(mc, "private:mark:timestamp", number_to_string(CURRTIME));

		wallops("\2%s\2 marked the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "MARK:ON: \2%s\2 (reason: \2%s\2)", mc->name, info);
		command_success_nodata(si, _("\2%s\2 is now marked."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not marked."), target);
			return;
		}

		metadata_delete(mc, "private:mark:setter");
		metadata_delete(mc, "private:mark:reason");
		metadata_delete(mc, "private:mark:timestamp");

		wallops("\2%s\2 unmarked the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "MARK:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("\2%s\2 is now unmarked."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_badparams, _("Usage: MARK <#channel> <ON|OFF> [note]"));
	}
}

static struct command cs_mark = {
	.name           = "MARK",
	.desc           = N_("Adds a note to a channel."),
	.access         = PRIV_MARK,
	.maxparc        = 3,
	.cmd            = &cs_cmd_mark,
	.help           = { .path = "cservice/mark" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_mark);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_mark);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/mark", MODULE_UNLOAD_CAPABILITY_OK)
