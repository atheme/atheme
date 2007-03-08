/*
 * Copyright (c) 2005-2006 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService COUNT functions.
 *
 * $Id: count.c 7929 2007-03-08 18:50:21Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/count", FALSE, _modinit, _moddeinit,
	"$Id: count.c 7929 2007-03-08 18:50:21Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_count(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_count = { "COUNT", N_("Shows number of entries in access lists."),
                         AC_NONE, 1, cs_cmd_count };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");
	help_addentry(cs_helptree, "COUNT", "help/cservice/count", NULL);
        command_add(&cs_count, cs_cmdtree);
}

void _moddeinit()
{
	help_delentry(cs_helptree, "COUNT");
	command_delete(&cs_count, cs_cmdtree);
}

static void cs_cmd_count(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	chanacs_t *ca;
	mychan_t *mc = mychan_find(chan);
	int vopcnt = 0, aopcnt = 0, hopcnt = 0, sopcnt = 0, akickcnt = 0;
	int othercnt = 0;
	int i;
	node_t *n;
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
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
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
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->level == chansvs.ca_vop)
			vopcnt++;
		else if (ca->level == chansvs.ca_hop)
			hopcnt++;
		else if (ca->level == chansvs.ca_aop)
			aopcnt++;
		else if (ca->level == chansvs.ca_sop)
			sopcnt++;
		else if (ca->level == CA_AKICK)
			akickcnt++;
		else if (ca->myuser != mc->founder)
			othercnt++;
	}
	command_success_nodata(si, _("%s: VOp: %d, HOp: %d, AOp: %d, SOp: %d, AKick: %d, other: %d"),
			chan, vopcnt, hopcnt, aopcnt, sopcnt, akickcnt, othercnt);
	snprintf(str, sizeof str, "%s: ", chan);
	for (i = 0; chanacs_flags[i].flag; i++)
	{
		othercnt = 0;
		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level & chanacs_flags[i].value)
				othercnt++;
		}
		snprintf(str + strlen(str), sizeof str - strlen(str),
				"%c:%d ", chanacs_flags[i].flag, othercnt);
	}
	command_success_nodata(si, "%s", str);
	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "%s COUNT (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "%s COUNT", mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
