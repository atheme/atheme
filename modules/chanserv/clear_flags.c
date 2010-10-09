/*
 * Copyright (c) 2010 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService CLEAR FLAGS functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_flags", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_flags(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_clear_flags = { "FLAGS", "Clears all channel flags.", AC_NONE, 2, cs_cmd_clear_flags, { .path = "cservice/clear_flags" } };

mowgli_patricia_t **cs_clear_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_clear_cmds, "chanserv/clear", "cs_clear_cmds");

        command_add(&cs_clear_flags, *cs_clear_cmds);
}

void _moddeinit()
{
	command_delete(&cs_clear_flags, *cs_clear_cmds);
}

static void cs_cmd_clear_flags(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	chanacs_t *ca;
	char *name = parv[0];

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

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 does not exist.", name);
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

		object_unref(ca);
	}

	logcommand(si, CMDLOG_DO, "CLEAR:FLAGS: \2%s\2", mc->name);
	command_success_nodata(si, _("Cleared flags in \2%s\2."), name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
