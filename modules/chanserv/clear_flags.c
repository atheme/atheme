/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the CService CLEAR FLAGS functions.
 */

#include <atheme.h>

static mowgli_patricia_t **cs_clear_cmds = NULL;

static void
cs_cmd_clear_flags(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc;
	mowgli_node_t *n, *tn;
	struct chanacs *ca;
	char *name = parv[0];
	unsigned int changes = 0;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEAR FLAGS");
		command_fail(si, fault_needmoreparams, _("Syntax: CLEAR <#channel> FLAGS"));
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, name);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_FOUNDER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, name);
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

	logcommand(si, CMDLOG_SET, "CLEAR:FLAGS: \2%s\2", mc->name);
	command_success_nodata(si, ngettext(N_("Cleared \2%u\2 access entry in \2%s\2."),
	                                    N_("Cleared \2%u\2 access entries in \2%s\2."),
	                                    changes), changes, name);
	if (changes > 0)
		verbose(mc, "\2%s\2 removed all %u non-founder access entries.", get_source_name(si), changes);
}

static struct command cs_clear_flags = {
	.name           = "FLAGS",
	.desc           = N_("Clears all channel flags."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_clear_flags,
	.help           = { .path = "cservice/clear_flags" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_clear_cmds, "chanserv/clear", "cs_clear_cmds")

        command_add(&cs_clear_flags, *cs_clear_cmds);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	command_delete(&cs_clear_flags, *cs_clear_cmds);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/clear_flags", MODULE_UNLOAD_CAPABILITY_OK)
