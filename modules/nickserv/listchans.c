/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the nickserv LISTCHANS function.
 *   -- Contains an alias "MYACCESS" for legacy users
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/listchans", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listchans(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_listchans = { "LISTCHANS", N_("Lists channels that you have access to."), AC_NONE, 1, ns_cmd_listchans };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_listchans, ns_cmdtree);
	help_addentry(ns_helptree, "LISTCHANS", "help/nickserv/listchans", NULL);
}

void _moddeinit()
{
	command_delete(&ns_listchans, ns_cmdtree);
	help_delentry(ns_helptree, "LISTCHANS");
}

static void ns_cmd_listchans(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	node_t *n;
	chanacs_t *ca;
	unsigned int akicks = 0, i;

	/* Optional target */
	char *target = parv[0];

	if (target)
	{
		if (!has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to use the target argument."));
			return;
		}

		mu = myuser_find_ext(target);

		if (mu == NULL)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
	}
	else
	{
		mu = si->smu;
		if (mu == NULL)
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}
	}

	if (mu != si->smu)
	{	/* must have been an oper */
		logcommand(si, CMDLOG_ADMIN, "LISTCHANS: \2%s\2", mu->name);
	}
	else
	{	/* just a user, or oper is listing himself */
		logcommand(si, CMDLOG_GET, "LISTCHANS");
	}

	if (mu->chanacs.count == 0)
	{
		command_success_nodata(si, _("No channel access was found for the nickname \2%s\2."), mu->name);
		return;
	}

	LIST_FOREACH(n, mu->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		/* don't tell users they're akicked (flag +b) */
		if (ca->level != CA_AKICK)
			command_success_nodata(si, _("Access flag(s) %s in %s"), bitmask_to_flags(ca->level, chanacs_flags), ca->mychan->name);
		else
			akicks++;
	}

	i = mu->chanacs.count - akicks;

	if (i == 0)
		command_success_nodata(si, _("No channel access was found for the nickname \2%s\2."), mu->name);
	else
		command_success_nodata(si, ngettext(N_("\2%d\2 channel access match for the nickname \2%s\2"),
						    N_("\2%d\2 channel access matches for the nickname \2%s\2"), i),
						    i, mu->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
