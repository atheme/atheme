/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the nickserv LISTCHANS function.
 *   -- Contains an alias "MYACCESS" for legacy users
 *
 * $Id: listchans.c 6457 2006-09-25 10:33:40Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/listchans", FALSE, _modinit, _moddeinit,
	"$Id: listchans.c 6457 2006-09-25 10:33:40Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_listchans(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_listchans = { "LISTCHANS", "Lists channels that you have access to.", AC_NONE, 1, ns_cmd_listchans };
command_t ns_myaccess = { "MYACCESS", "Alias for LISTCHANS", AC_NONE, 1, ns_cmd_listchans };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_myaccess, ns_cmdtree);
	help_addentry(ns_helptree, "MYACCESS", "help/nickserv/listchans", NULL);

	command_add(&ns_listchans, ns_cmdtree);
	help_addentry(ns_helptree, "LISTCHANS", "help/nickserv/listchans", NULL);
}

void _moddeinit()
{
	command_delete(&ns_myaccess, ns_cmdtree);
	help_delentry(ns_helptree, "MYACCESS");

	command_delete(&ns_listchans, ns_cmdtree);
	help_delentry(ns_helptree, "LISTCHANS");
}

static void ns_cmd_listchans(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	node_t *n;
	chanacs_t *ca;
	uint32_t akicks = 0, i;

	/* Optional target */
	char *target = parv[0];

	if (target)
	{
		if (!has_priv(si->su, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_authfail, "You are not authorized to use the target argument.");
			return;
		}

		mu = myuser_find_ext(target);

		if (mu == NULL)
		{
			command_fail(si, fault_nosuch_target, "The nickname \2%s\2 is not registered.", target);
			return;
		}
	}
	else
	{
		mu = si->su->myuser;
		if (mu == NULL)
		{
			command_fail(si, fault_authfail, "You are not logged in.");
			return;
		}
	}

	if (mu != si->su->myuser)
	{	/* must have been an oper */
		snoop("LISTCHANS: \2%s\2 on \2%s\2", si->su->nick, mu->name);
		logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "LISTCHANS %s", mu->name);
	}
	else
	{	/* just a user, or oper is listing himself */
		logcommand(nicksvs.me, si->su, CMDLOG_GET, "LISTCHANS");
	}

	if (mu->chanacs.count == 0)
	{
		command_success_nodata(si, "No channel access was found for the nickname \2%s\2.", mu->name);
		return;
	}

	LIST_FOREACH(n, mu->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (is_founder(ca->mychan, mu))
			command_success_nodata(si, "Founder of %s", ca->mychan->name);

		switch (ca->level)
		{
			default:
				/* don't tell users they're akicked (flag +b) */
				if (!(ca->level & CA_AKICK))
					command_success_nodata(si, "Access flag(s) %s in %s", bitmask_to_flags(ca->level, chanacs_flags), ca->mychan->name);
				else
					akicks++;
		}
	}

	i = mu->chanacs.count - akicks;

	if (i == 0)
		command_success_nodata(si, "No channel access was found for the nickname \2%s\2.", mu->name);
	else
		command_success_nodata(si, "\2%d\2 channel access match%s for the nickname \2%s\2",
							i, (i > 1) ? "es" : "", mu->name);
}
