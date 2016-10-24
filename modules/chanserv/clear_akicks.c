/*
 * Copyright (c) 2014 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService CLEAR FLAGS functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_akicks", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

static void cs_cmd_clear_akicks(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_clear_akicks = { "AKICKS", "Clears all channel AKICK entries.", AC_NONE, 2, cs_cmd_clear_akicks, { .path = "cservice/clear_akicks" } };

mowgli_patricia_t **cs_clear_cmds;

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_clear_cmds, "chanserv/clear", "cs_clear_cmds");

        command_add(&cs_clear_akicks, *cs_clear_cmds);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&cs_clear_akicks, *cs_clear_cmds);
}

static void cs_cmd_clear_akicks(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	chanacs_t *ca;
	char *name = parv[0];
	int changes = 0;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEAR AKICKS");
		command_fail(si, fault_needmoreparams, "Syntax: CLEAR <#channel> AKICKS");
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

	if (!(chanacs_source_has_flag(mc, si, CA_RECOVER) && chanacs_source_has_flag(mc, si, CA_FLAGS)))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}


	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
	{
		ca = n->data;

		if (ca->level != CA_AKICK)
			continue;

		changes++;
		object_unref(ca);
	}

	logcommand(si, CMDLOG_DO, "CLEAR:AKICKS: \2%s\2", mc->name);
	command_success_nodata(si, _("Cleared AKICK entries in \2%s\2."), name);
	if (changes > 0)
		verbose(mc, _("\2%s\2 removed all %d AKICK entries."),
				get_source_name(si), changes);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
