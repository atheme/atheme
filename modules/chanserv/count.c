/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Patrick Fish, et al.
 *
 * This file contains code for the CService COUNT functions.
 */

#include <atheme.h>

static void
cs_cmd_count(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan = parv[0];
	struct chanacs *ca;
	struct mychan *mc = mychan_find(chan);
	unsigned int ca_sop, ca_aop, ca_hop, ca_vop;
	unsigned int vopcnt = 0, aopcnt = 0, hopcnt = 0, sopcnt = 0, akickcnt = 0;
	unsigned int othercnt = 0;
	unsigned int i;
	mowgli_node_t *n;
	char str[512];
	bool operoverride = false;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "COUNT");
		command_fail(si, fault_needmoreparams, _("Syntax: COUNT <#channel>"));
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, chan);
		return;
	}

	if (!(mc->flags & MC_PUBACL) && !chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = true;
		else
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, STR_CHANNEL_IS_CLOSED, chan);
		return;
	}

	ca_sop = get_template_flags(mc, "SOP");
	ca_aop = get_template_flags(mc, "AOP");
	ca_hop = get_template_flags(mc, "HOP");
	ca_vop = get_template_flags(mc, "VOP");

	bool show_akicks = true;

	if (chansvs.hide_pubacl_akicks)
		show_akicks = ( chanacs_source_has_flag(mc, si, CA_ACLVIEW) || has_priv(si, PRIV_CHAN_AUSPEX) );

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = (struct chanacs *)n->data;

		if (ca->level == ca_vop)
			vopcnt++;
		else if (ca->level == ca_hop)
			hopcnt++;
		else if (ca->level == ca_aop)
			aopcnt++;
		else if (ca->level == ca_sop)
			sopcnt++;
		else if (ca->level == CA_AKICK)
			akickcnt++;
		else
			othercnt++;
	}
	if (show_akicks)
	{
		if (ca_hop == ca_vop)
			command_success_nodata(si, _("%s: VOP: %u, AOP: %u, SOP: %u, AKICK: %u, Other: %u"),
					chan, vopcnt, aopcnt, sopcnt, akickcnt, othercnt);
		else
			command_success_nodata(si, _("%s: VOP: %u, HOP: %u, AOP: %u, SOP: %u, AKICK: %u, Other: %u"),
					chan, vopcnt, hopcnt, aopcnt, sopcnt, akickcnt, othercnt);
	}
	else
	{
		if (ca_hop == ca_vop)
			command_success_nodata(si, _("%s: VOP: %u, AOP: %u, SOP: %u, Other: %u"),
					chan, vopcnt, aopcnt, sopcnt, othercnt);
		else
			command_success_nodata(si, _("%s: VOP: %u, HOP: %u, AOP: %u, SOP: %u, Other: %u"),
					chan, vopcnt, hopcnt, aopcnt, sopcnt, othercnt);
	}
	snprintf(str, sizeof str, "%s: ", chan);
	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
	{
		if (!(ca_all & chanacs_flags[i].value))
			continue;
		if (chanacs_flags[i].value == CA_AKICK && !show_akicks)
			continue;
		othercnt = 0;
		MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
		{
			ca = (struct chanacs *)n->data;

			if (ca->level & chanacs_flags[i].value)
				othercnt++;
		}
		snprintf(str + strlen(str), sizeof str - strlen(str),
				"%c:%u ", (char) i, othercnt);
	}
	command_success_nodata(si, "%s", str);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "COUNT: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "COUNT: \2%s\2", mc->name);
}

static struct command cs_count = {
	.name           = "COUNT",
	.desc           = N_("Shows number of entries in access lists."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &cs_cmd_count,
	.help           = { .path = "cservice/count" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "chanserv/main")

        service_named_bind_command("chanserv", &cs_count);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_count);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/count", MODULE_UNLOAD_CAPABILITY_OK)
