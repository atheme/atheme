/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Lists object properties via their metadata table.
 */

#include <atheme.h>

static void
cs_cmd_taxonomy(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	struct mychan *mc;
	mowgli_patricia_iteration_state_t state;
	struct metadata *md;
	bool isoper;

	if (!target || *target != '#')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TAXONOMY");
		command_fail(si, fault_needmoreparams, _("Syntax: TAXONOMY <#channel>"));
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered channel."), target);
		return;
	}

	isoper = has_priv(si, PRIV_CHAN_AUSPEX);

	if ((mc->flags & MC_PRIVATE) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW) && !isoper)
	{
		command_fail(si, fault_noprivs, _("Channel \2%s\2 is private."),
				mc->name);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, target);
		return;
	}

	if (isoper)
		logcommand(si, CMDLOG_ADMIN, "TAXONOMY: \2%s\2 (oper)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "TAXONOMY: \2%s\2", mc->name);
	command_success_nodata(si, _("Taxonomy for \2%s\2:"), target);

	MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mc)->metadata)
	{
                if (!strncmp(md->name, "private:", 8) && !isoper)
                        continue;

		command_success_nodata(si, "%-32s: %s", md->name, md->value);
	}

	command_success_nodata(si, _("End of \2%s\2 taxonomy."), target);
}

static struct command cs_taxonomy = {
	.name           = "TAXONOMY",
	.desc           = N_("Displays a channel's metadata."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_taxonomy,
	.help           = { .path = "cservice/taxonomy" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_taxonomy);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_taxonomy);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/taxonomy", MODULE_UNLOAD_CAPABILITY_OK)
