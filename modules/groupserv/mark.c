/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2021 Atheme Project (http://atheme.org/)
 *
 * Marking for channels.
 */

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_mark(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	struct mygroup *mg;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("Usage: MARK <!group> <ON|OFF> [note]"));
		return;
	}

	if (!(mg = mygroup_find(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <!group> ON <note>"));
			return;
		}

		if (metadata_find(mg, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already marked."), target);
			return;
		}

		metadata_add(mg, "private:mark:setter", get_oper_name(si));
		metadata_add(mg, "private:mark:reason", info);
		metadata_add(mg, "private:mark:timestamp", number_to_string(CURRTIME));

		wallops("\2%s\2 marked the group \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "MARK:ON: \2%s\2 (reason: \2%s\2)", entity(mg)->name, info);
		command_success_nodata(si, _("\2%s\2 is now marked."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mg, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not marked."), target);
			return;
		}

		metadata_delete(mg, "private:mark:setter");
		metadata_delete(mg, "private:mark:reason");
		metadata_delete(mg, "private:mark:timestamp");

		wallops("\2%s\2 unmarked the group \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "MARK:OFF: \2%s\2", entity(mg)->name);
		command_success_nodata(si, _("\2%s\2 is now unmarked."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_badparams, _("Usage: MARK <!group> <ON|OFF> [note]"));
	}
}

static struct command gs_mark = {
	.name           = "MARK",
	.desc           = N_("Adds a note to a group."),
	.access         = PRIV_MARK,
	.maxparc        = 3,
	.cmd            = &gs_cmd_mark,
	.help           = { .path = "groupserv/mark" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_mark);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_mark);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/mark", MODULE_UNLOAD_CAPABILITY_OK)
