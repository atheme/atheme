/*
 * Copyright (c) 2005-2006 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService COUNT functions.
 *
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/count", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_count(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_count = { "COUNT", N_("Shows number of entries in access lists."),
                         AC_NONE, 1, cs_cmd_count, { .path = "cservice/count" } };

void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_count);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_count);
}

static void cs_cmd_count(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	chanacs_t *ca;
	mychan_t *mc = mychan_find(chan);
	unsigned int ca_sop, ca_aop, ca_hop, ca_vop;
	int vopcnt = 0, aopcnt = 0, hopcnt = 0, sopcnt = 0, akickcnt = 0;
	int othercnt = 0;
	unsigned int i;
	mowgli_node_t *n;
	char str[512];
	int operoverride = 0;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "COUNT");
		command_fail(si, fault_needmoreparams, _("Syntax: COUNT <#channel>"));
		return;
	}

	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = 1;
		else
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	ca_sop = get_template_flags(mc, "SOP");
	ca_aop = get_template_flags(mc, "AOP");
	ca_hop = get_template_flags(mc, "HOP");
	ca_vop = get_template_flags(mc, "VOP");

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

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
	if (ca_hop == ca_vop)
		command_success_nodata(si, _("%s: VOP: %d, AOP: %d, SOP: %d, AKick: %d, Other: %d"),
				chan, vopcnt, aopcnt, sopcnt, akickcnt, othercnt);
	else
		command_success_nodata(si, _("%s: VOP: %d, HOP: %d, AOP: %d, SOP: %d, AKick: %d, Other: %d"),
				chan, vopcnt, hopcnt, aopcnt, sopcnt, akickcnt, othercnt);
	snprintf(str, sizeof str, "%s: ", chan);
	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
	{
		if (!(ca_all & chanacs_flags[i].value))
			continue;
		othercnt = 0;
		MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level & chanacs_flags[i].value)
				othercnt++;
		}
		snprintf(str + strlen(str), sizeof str - strlen(str),
				"%c:%d ", (char) i, othercnt);
	}
	command_success_nodata(si, "%s", str);
	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "COUNT: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "COUNT: \2%s\2", mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
