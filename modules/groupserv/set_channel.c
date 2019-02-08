/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static const struct groupserv_core_symbols *gcsyms = NULL;
static mowgli_patricia_t **gs_set_cmdtree = NULL;

static void
gs_cmd_set_channel_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET CHANNEL");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: SET CHANNEL <!group> [channel]"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET CHANNEL");
		(void) command_fail(si, fault_badparams, _("Syntax: SET CHANNEL <!group> [channel]"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), group);
		return;
	}

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		(void) command_fail(si, fault_noprivs, _("You are not authorized to execute this command."));
		return;
	}

	const char *const param = parv[1];

	if (! param || strcasecmp("OFF", param) == 0 || strcasecmp("NONE", param) == 0)
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "channel"))
		{
			(void) metadata_delete(mg, "channel");
			(void) logcommand(si, CMDLOG_SET, "SET:CHANNEL:NONE: \2%s\2", group);
			(void) command_success_nodata(si, _("The official channel for \2%s\2 has been cleared."), group);
			return;
		}

		(void) command_fail(si, fault_nochange, _("A official channel for \2%s\2 was not set."), group);
		return;
	}

	// we'll overwrite any existing metadata
	(void) metadata_add(mg, "channel", param);

	(void) logcommand(si, CMDLOG_SET, "SET:CHANNEL: \2%s\2 \2%s\2", group, param);
	(void) command_success_nodata(si, _("The official channel of \2%s\2 has been set to \2%s\2."), group, param);
}

static struct command gs_cmd_set_channel = {
	.name           = "CHANNEL",
	.desc           = N_("Sets the official group channel."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_channel_func,
	.help           = { .path = "groupserv/set_channel" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");
	MODULE_TRY_REQUEST_SYMBOL(m, gs_set_cmdtree, "groupserv/set", "gs_set_cmdtree");

	(void) command_add(&gs_cmd_set_channel, *gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) command_delete(&gs_cmd_set_channel, *gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_channel", MODULE_UNLOAD_CAPABILITY_OK)
