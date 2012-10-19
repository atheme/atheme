/*
 * Copyright (c) 2010 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService CLONE functions.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clone", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clone(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_clone = { "CLONE", "Clones a channel.", AC_NONE, 2, cs_cmd_clone, { .path = "cservice/clone" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_clone);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_clone);
}

static void cs_cmd_clone(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc, *mc2;
	mowgli_node_t *n, *tn;
	mowgli_patricia_iteration_state_t state;
	metadata_t *md;
	chanacs_t *ca;
	char *source = parv[0];
	char *target = parv[1];

	if (!source || !target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLONE");
		command_fail(si, fault_needmoreparams, "Syntax: CLONE <#source> <#target>");
		return;
	}

	if (!(mc = mychan_find(source)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", source);
		return;
	}
	
	if (!(mc2 = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	if (mc == mc2)
	{
		command_fail(si, fault_nochange, "Cannot clone a channel to itself.");
		return;
	}
	
	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", source);
		return;
	}
	
	if (metadata_find(mc2, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", target);
		return;
	}

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 does not exist.", source);
		return;
	}
	
	if (!mc2->chan)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 does not exist.", target);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (!chanacs_source_has_flag(mc2, si, CA_FOUNDER))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	/* Delete almost all chanacs of the target first */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mc2->chanacs.head)
	{
		ca = n->data;

		if (ca->level & CA_FOUNDER)
			continue;

		object_unref(ca);
	}

	/* Copy source chanacs to target */
	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;

		if (!ca->host)
			chanacs_change_simple(mc2, ca->entity, NULL, ca->level, 0, myentity_find(ca->setter));
		else if (ca->host != NULL)
			chanacs_change_simple(mc2, NULL, ca->host, ca->level, 0, myentity_find(ca->setter));
	}

	/* Copy ze metadata! */
	MOWGLI_PATRICIA_FOREACH(md, &state, object(mc)->metadata)
	{
		if(!strncmp(md->name, "private:topic:", 14))
				continue;

		metadata_add(mc2, md->name, md->value);
	}

	/* Copy channel flags */
	mc2->flags = mc->flags;

	command_add_flood(si, FLOOD_MODERATE);

	/* I feel like this should log at a higher level... */
	logcommand(si, CMDLOG_DO, "CLONE: \2%s\2 to \2%s\2", mc->name, mc2->name);
	command_success_nodata(si, _("Cloned \2%s\2 to \2%s\2."), source, target);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
