/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2016 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the CService CLONE functions.
 */

#include <atheme.h>

static void
cs_cmd_clone(struct sourceinfo *si, int parc, char *parv[])
{
	struct mychan *mc, *mc2;
	mowgli_node_t *n, *tn;
	mowgli_patricia_iteration_state_t state;
	struct metadata *md;
	struct chanacs *ca;
	char *source = parv[0];
	char *target = parv[1];

	if (!source || !target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONE");
		command_fail(si, fault_needmoreparams, _("Syntax: CLONE <#source> <#target>"));
		return;
	}

	if (!(mc = mychan_find(source)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, source);
		return;
	}

	if (!(mc2 = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (mc == mc2)
	{
		command_fail(si, fault_nochange, _("You cannot clone a channel to itself."));
		return;
	}

	if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (!chanacs_source_has_flag(mc2, si, CA_FOUNDER))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, source);
		return;
	}

	if (metadata_find(mc2, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, target);
		return;
	}

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, source);
		return;
	}

	if (!mc2->chan)
	{
		command_fail(si, fault_nosuch_target, STR_CHANNEL_IS_EMPTY, target);
		return;
	}

	// Delete almost all chanacs of the target first
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc2->chanacs.head)
	{
		ca = n->data;

		if (ca->level & CA_FOUNDER)
			continue;

		atheme_object_unref(ca);
	}

	// Copy source chanacs to target
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;

		if (!ca->host)
			chanacs_change_simple(mc2, ca->entity, NULL, ca->level, 0, *ca->setter_uid != '\0' ? myentity_find_uid(ca->setter_uid) : NULL);
		else if (ca->host != NULL)
			chanacs_change_simple(mc2, NULL, ca->host, ca->level, 0, *ca->setter_uid != '\0' ? myentity_find_uid(ca->setter_uid) : NULL);
	}

	// Copy ze metadata!
	MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mc)->metadata)
	{
		if(!strncmp(md->name, "private:topic:", 14))
		{
			continue;
		}

		// Replace ANTIFLOOD AKILL with QUIET if it exists --shaynejellesma
		if((strcasecmp(md->name, "private:antiflood:enforce-method") == 0) && (strcasecmp(md->value, "AKILL") == 0))
		{
			metadata_add(mc2, md->name, "QUIET");
			continue;
		}

		metadata_add(mc2, md->name, md->value);
	}

	// Copy channel flags
	mc2->flags = mc->flags;

	// Remove HOLD flag if it exists --shaynejellesma
	if (mc2->flags & MC_HOLD)
		mc2->flags &= ~MC_HOLD;

	command_add_flood(si, FLOOD_MODERATE);

	logcommand(si, CMDLOG_SET, "CLONE: \2%s\2 to \2%s\2", mc->name, mc2->name);
	command_success_nodata(si, _("Cloned \2%s\2 to \2%s\2."), source, target);
}

static struct command cs_clone = {
	.name           = "CLONE",
	.desc           = N_("Clones a channel."),
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &cs_cmd_clone,
	.help           = { .path = "cservice/clone" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

	service_named_bind_command("chanserv", &cs_clone);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_clone);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/clone", MODULE_UNLOAD_CAPABILITY_OK)
