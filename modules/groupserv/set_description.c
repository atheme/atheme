/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains routines to handle the GroupServ HELP command.
 */

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_set_description(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	char *desc = parv[1];

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_SET))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!desc || !strcasecmp("OFF", desc) || !strcasecmp("NONE", desc))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "description"))
		{
			metadata_delete(mg, "description");
			logcommand(si, CMDLOG_SET, "SET:DESCRIPTION:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The description for \2%s\2 has been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("A description for \2%s\2 was not set."), parv[0]);
		return;
	}

	// we'll overwrite any existing metadata
	metadata_add(mg, "description", desc);

	logcommand(si, CMDLOG_SET, "SET:DESCRIPTION: \2%s\2 \2%s\2", entity(mg)->name, desc);
	command_success_nodata(si, _("The description of \2%s\2 has been set to \2%s\2."), parv[0], desc);
}

static struct command gs_set_description = {
	.name           = "DESCRIPTION",
	.desc           = N_("Sets the group description."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_description,
	.help           = { .path = "groupserv/set_description" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_description, gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&gs_set_description, gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_description", MODULE_UNLOAD_CAPABILITY_OK)
