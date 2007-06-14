/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ WHY function.
 *
 * $Id: why.c 8209 2007-04-30 00:14:52Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/why", FALSE, _modinit, _moddeinit,
	"$Id: why.c 8209 2007-04-30 00:14:52Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_why(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_why = { "WHY", N_("Explains channel access logic."),
		     AC_NONE, 2, cs_cmd_why };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_why, cs_cmdtree);
	help_addentry(cs_helptree, "WHY", "help/cservice/why", NULL);
}

void _moddeinit()
{
	command_delete(&cs_why, cs_cmdtree);
	help_delentry(cs_helptree, "WHY");
}

static void cs_cmd_why(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *targ = parv[1];
	char host[BUFSIZE];
	mychan_t *mc;
	user_t *u;
	myuser_t *mu;
	node_t *n;
	chanacs_t *ca;
	metadata_t *md;
	int operoverride = 0;
	int fl = 0;

	if (!chan || !targ)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "WHY");
		command_fail(si, fault_needmoreparams, _("Syntax: WHY <channel> <user>"));
		return;
	}

	mc = mychan_find(chan);
	u = user_find_named(targ);

	if (u == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."),
			targ);
		return;
	}

	mu = u->myuser;

	if (mc == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."),
			chan);
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

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "%s WHY %s!%s@%s (oper override)", mc->name, u->nick, u->user, u->vhost);
	else
		logcommand(si, CMDLOG_GET, "%s WHY %s!%s@%s", mc->name, u->nick, u->user, u->vhost);

	snprintf(host, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->vhost);

	if (is_founder(mc, mu))
	{
		command_success_nodata(si,
			"\2%s\2 has founder access to \2%s\2 because they are logged in as \2%s\2.",
			u->nick, mc->name, mu->name);
	}
	LIST_FOREACH(n, mc->chanacs.head)
	{
       	        ca = (chanacs_t *)n->data;

		if (ca->myuser != NULL && ca->myuser == mu)
		{
			fl |= ca->level;
			command_success_nodata(si,
				"\2%s\2 has flags \2%s\2 in \2%s\2 because they are logged in as \2%s\2.",
				u->nick, bitmask_to_flags2(ca->level, 0, chanacs_flags), mc->name, mu->name);
			if (ca->level & CA_AKICK)
			{
				md = metadata_find(ca, METADATA_CHANACS, "reason");
				if (md != NULL)
					command_success_nodata(si, "Ban reason: %s", md->value);
			}
		}
	}
	for (n = next_matching_host_chanacs(mc, u, mc->chanacs.head); n != NULL; n = next_matching_host_chanacs(mc, u, n->next))
	{
		ca = n->data;
		fl |= ca->level;
		command_success_nodata(si,
				"\2%s\2 has flags \2%s\2 in \2%s\2 because they match the mask \2%s\2.",
				u->nick, bitmask_to_flags2(ca->level, 0, chanacs_flags), mc->name, ca->host);
		if (ca->level & CA_AKICK)
		{
			md = metadata_find(ca, METADATA_CHANACS, "reason");
			if (md != NULL)
				command_success_nodata(si, "Ban reason: %s", md->value);
		}
	}

	if ((fl & (CA_AKICK | CA_REMOVE)) == (CA_AKICK | CA_REMOVE))
		command_success_nodata(si, _("+r exempts from +b."));
	else if (fl == 0)
		command_success_nodata(si, _("\2%s\2 has no special access to \2%s\2."),
				u->nick, mc->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
