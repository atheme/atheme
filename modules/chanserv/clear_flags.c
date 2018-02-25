/*
 * Copyright (c) 2010 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService CLEAR FLAGS functions.
 */

#include "atheme.h"

static void cs_cmd_clear_flags(struct sourceinfo *si, int parc, char *parv[]);

static struct command cs_clear_flags = { "FLAGS", "Clears all channel flags.", AC_NONE, 2, cs_cmd_clear_flags, { .path = "cservice/clear_flags" } };

static mowgli_patricia_t **cs_clear_cmds = NULL;

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_clear_cmds, "chanserv/clear", "cs_clear_cmds");

        command_add(&cs_clear_flags, *cs_clear_cmds);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_clear_flags, *cs_clear_cmds);
}

static void
cs_cmd_clear_flags(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	mowgli_node_t *n, *tn;
	struct chanacs *ca;
	char *name = parv[0];
	int changes = 0;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEAR FLAGS");
		command_fail(si, fault_needmoreparams, "Syntax: CLEAR <#channel> FLAGS");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", name);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", name);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_FOUNDER))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}


	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
	{
		ca = n->data;

		if (ca->level & CA_FOUNDER)
			continue;

		changes++;
		atheme_object_unref(ca);
	}

	logcommand(si, CMDLOG_DO, "CLEAR:FLAGS: \2%s\2", mc->name);
	command_success_nodata(si, _("Cleared flags in \2%s\2."), name);
	if (changes > 0)
		verbose(mc, _("\2%s\2 removed all %d non-founder access entries."),
				get_source_name(si), changes);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/clear_flags", MODULE_UNLOAD_CAPABILITY_OK)
