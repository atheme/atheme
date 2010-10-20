/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FTRANSFER function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ftransfer", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ftransfer(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_ftransfer = { "FTRANSFER", N_("Forces foundership transfer of a channel."),
                           PRIV_CHAN_ADMIN, 2, cs_cmd_ftransfer, { .path = "cservice/ftransfer" } };
                                                                                   
void _modinit(module_t *m)
{
        service_named_bind_command("chanserv", &cs_ftransfer);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_ftransfer);
}

static void cs_cmd_ftransfer(sourceinfo_t *si, int parc, char *parv[])
{
	myentity_t *mt;
	mychan_t *mc;
	mowgli_node_t *n;
	chanacs_t *ca;
	char *name = parv[0];
	char *newfndr = parv[1];
	const char *oldfndr;

	if (!name || !newfndr)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FTRANSFER");
		command_fail(si, fault_needmoreparams, _("Syntax: FTRANSFER <#channel> <newfounder>"));
		return;
	}

	if (!(mt = myentity_find_ext(newfndr)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), newfndr);
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), name);
		return;
	}
	
	oldfndr = mychan_founder_names(mc);
	if (!strcmp(mt->name, oldfndr))
	{
		command_fail(si, fault_nochange, _("\2%s\2 is already the founder of \2%s\2."), mt->name, name);
		return;
	}

	/* no maxchans check (intentional -- this is an oper command) */
	wallops("%s transferred foundership of %s from %s to %s", get_oper_name(si), name, oldfndr, mt->name);
	logcommand(si, CMDLOG_ADMIN | LG_REGISTER, "FTRANSFER: \2%s\2 transferred from \2%s\2 to \2%s\2", mc->name, oldfndr, mt->name);
	verbose(mc, "Foundership transfer from \2%s\2 to \2%s\2 forced by %s administration.", oldfndr, mt->name, me.netname);
	command_success_nodata(si, _("Foundership of \2%s\2 has been transferred from \2%s\2 to \2%s\2."),
		name, oldfndr, mt->name);

	MOWGLI_ITER_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		/* CA_FLAGS is always on if CA_FOUNDER is on, this just
		 * ensures we don't crash if not -- jilles
		 */
		if (ca->entity != NULL && ca->level & CA_FOUNDER)
			chanacs_modify_simple(ca, CA_FLAGS, CA_FOUNDER);
	}
	mc->used = CURRTIME;
	chanacs_change_simple(mc, mt, NULL, CA_FOUNDER_0, 0);

	/* delete transfer metadata -- prevents a user from stealing it back */
	metadata_delete(mc, "private:verify:founderchg:newfounder");
	metadata_delete(mc, "private:verify:founderchg:timestamp");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
