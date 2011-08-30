/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the GroupServ LISTCHANS function.
 *
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/listchans", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void gs_cmd_listchans(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_listchans = { "LISTCHANS", N_("Lists channels that a group has access to."), AC_NONE, 1, gs_cmd_listchans, { .path = "groupserv/listchans" } };

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);
	service_named_bind_command("groupserv", &gs_listchans);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_listchans);
}

static void gs_cmd_listchans(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;
	mowgli_node_t *n;
	chanacs_t *ca;
	bool operoverride = false;
	unsigned int akicks = 0, i;

	/* target */
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
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
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
		ca = (chanacs_t *)n->data;

		/* don't tell users they're akicked (flag +b) */
		if (ca->level != CA_AKICK)
			command_success_nodata(si, _("Access flag(s) %s in %s"), bitmask_to_flags(ca->level), ca->mychan->name);
		else
			akicks++;
	}

	i = MOWGLI_LIST_LENGTH(&entity(mg)->chanacs) - akicks;

	if (i == 0)
		command_success_nodata(si, _("No channel access was found for the group \2%s\2."), entity(mg)->name);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 channel access match for the group \2%s\2"),
						    N_("\2%d\2 channel access matches for the group \2%s\2"), i),
						    i, entity(mg)->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
