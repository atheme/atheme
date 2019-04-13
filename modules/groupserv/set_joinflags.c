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
gs_cmd_set_joinflags(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	char *joinflags = parv[1];
	unsigned int flags = 0;

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

	if (!joinflags || !strcasecmp("OFF", joinflags) || !strcasecmp("NONE", joinflags))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mg, "joinflags"))
		{
			metadata_delete(mg, "joinflags");
			logcommand(si, CMDLOG_SET, "SET:JOINFLAGS:NONE: \2%s\2", entity(mg)->name);
			command_success_nodata(si, _("The group-specific join flags for \2%s\2 have been cleared."), parv[0]);
			return;
		}

		command_fail(si, fault_nochange, _("Join flags for \2%s\2 were not set."), parv[0]);
		return;
	}

	if (!strncasecmp(joinflags, "-", 1))
	{
		command_fail(si, fault_badparams, _("You can't set join flags to be removed."));
		return;
	}

	flags = gs_flags_parser(joinflags, 0, flags);

	// we'll overwrite any existing metadata
	metadata_add(mg, "joinflags", number_to_string(flags));

	logcommand(si, CMDLOG_SET, "SET:JOINFLAGS: \2%s\2 \2%s\2", entity(mg)->name, joinflags);
	command_success_nodata(si, _("The join flags of \2%s\2 have been set to \2%s\2."), parv[0], joinflags);
}

static struct command gs_set_joinflags = {
	.name           = "JOINFLAGS",
	.desc           = N_("Sets the flags users will be given when they JOIN the group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &gs_cmd_set_joinflags,
	.help           = { .path = "groupserv/set_joinflags" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);
	use_groupserv_set_symbols(m);

	command_add(&gs_set_joinflags, gs_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&gs_set_joinflags, gs_set_cmdtree);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/set_joinflags", MODULE_UNLOAD_CAPABILITY_OK)
