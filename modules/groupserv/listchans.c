/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 */

#include "atheme.h"

static const struct groupserv_core_symbols *gcsyms = NULL;

static void
gs_cmd_listchans_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc != 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LISTCHANS");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: LISTCHANS <!group>"));
		return;
	}

	const char *const group = parv[0];

	if (*group != '!')
	{
		(void) command_fail(si, fault_badparams, STR_INVALID_PARAMS, "LISTCHANS");
		(void) command_fail(si, fault_badparams, _("Syntax: LISTCHANS <!group>"));
		return;
	}

	struct mygroup *mg;

	if (! (mg = gcsyms->mygroup_find(group)))
	{
		(void) command_fail(si, fault_nosuch_target, _("Group \2%s\2 does not exist."), group);
		return;
	}

	bool operoverride = false;

	if (! gcsyms->groupacs_sourceinfo_has_flag(mg, si, GA_CHANACS))
	{
		if (! has_priv(si, PRIV_GROUP_AUSPEX))
		{
			(void) command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}

		operoverride = true;
	}

	if (operoverride)
		(void) logcommand(si, CMDLOG_ADMIN, "LISTCHANS: \2%s\2", group);
	else
		(void) logcommand(si, CMDLOG_GET, "LISTCHANS: \2%s\2", group);

	if (MOWGLI_LIST_LENGTH(&entity(mg)->chanacs) == 0)
	{
		(void) command_success_nodata(si, _("No channel access was found for the group \2%s\2."), group);
		return;
	}

	mowgli_node_t *n;
	unsigned int akicks = 0;

	MOWGLI_ITER_FOREACH(n, entity(mg)->chanacs.head)
	{
		const struct chanacs *const ca = n->data;

		// Don't tell users they're akicked (flag +b)
		if (ca->level != CA_AKICK)
			(void) command_success_nodata(si, _("Access flag(s) %s in %s"), bitmask_to_flags(ca->level),
			                              ca->mychan->name);
		else
			akicks++;
	}

	const unsigned int count = MOWGLI_LIST_LENGTH(&entity(mg)->chanacs) - akicks;

	if (! count)
	{
		(void) command_success_nodata(si, _("No channel access was found for the group \2%s\2."), group);
		return;
	}

	(void) command_success_nodata(si, ngettext(N_("\2%u\2 channel access match for the group \2%s\2"),
	                                           N_("\2%u\2 channel access matches for the group \2%s\2"), count),
	                                           count, group);
}

static struct command gs_cmd_listchans = {
	.name           = "LISTCHANS",
	.desc           = N_("Lists channels that a group has access to."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &gs_cmd_listchans_func,
	.help           = { .path = "groupserv/listchans" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, gcsyms, "groupserv/main", "groupserv_core_symbols");

	(void) service_bind_command(*gcsyms->groupsvs, &gs_cmd_listchans);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_unbind_command(*gcsyms->groupsvs, &gs_cmd_listchans);
}

SIMPLE_DECLARE_MODULE_V1("groupserv/listchans", MODULE_UNLOAD_CAPABILITY_OK)
