/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the GroupServ LISTCHANS function.
 */

#include <atheme.h>
#include "groupserv.h"

static void
gs_cmd_listchans(struct sourceinfo *si, int parc, char *parv[])
{
	struct mygroup *mg;
	mowgli_node_t *n;
	struct chanacs *ca;
	bool operoverride = false;
	unsigned int akicks = 0, i;

	// target
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LISTCHANS");
		command_fail(si, fault_needmoreparams, _("Syntax: LISTCHANS <!groupname>"));
		return;
	}

	if (!(mg = mygroup_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), target);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_CHANACS))
	{
		if (has_priv(si, PRIV_GROUP_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "LISTCHANS: \2%s\2", entity(mg)->name);
	else
		logcommand(si, CMDLOG_GET, "LISTCHANS: \2%s\2", entity(mg)->name);

	if (MOWGLI_LIST_LENGTH(&entity(mg)->chanacs) == 0)
	{
		command_success_nodata(si, _("No channel access was found for the group \2%s\2."), entity(mg)->name);
		return;
	}

	MOWGLI_ITER_FOREACH(n, entity(mg)->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		// don't tell users they're akicked (flag +b)
		if (ca->level != CA_AKICK)
			command_success_nodata(si, _("Access flag(s) \2%s\2 in \2%s\2"), bitmask_to_flags(ca->level), ca->mychan->name);
		else
			akicks++;
	}

	i = MOWGLI_LIST_LENGTH(&entity(mg)->chanacs) - akicks;

	if (i == 0)
		command_success_nodata(si, _("No channel access was found for the group \2%s\2."), entity(mg)->name);
	else
		command_success_nodata(si, ngettext(N_("\2%u\2 channel access match for the group \2%s\2"),
						    N_("\2%u\2 channel access matches for the group \2%s\2"), i),
						    i, entity(mg)->name);
}

static struct command gs_listchans = {
	.name           = "LISTCHANS",
	.desc           = N_("Lists channels that a group has access to."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &gs_cmd_listchans,
	.help           = { .path = "groupserv/listchans" },
};

static void
mod_init(struct module *const restrict m)
{
	use_groupserv_main_symbols(m);
	service_named_bind_command("groupserv", &gs_listchans);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("groupserv", &gs_listchans);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/listchans", MODULE_UNLOAD_CAPABILITY_OK)
