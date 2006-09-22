/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ MYACCESS function.
 *
 * $Id: why.c 6427 2006-09-22 19:38:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/why", FALSE, _modinit, _moddeinit,
	"$Id: why.c 6427 2006-09-22 19:38:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_why(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_why = {
	"WHY",
	"Explains channel access logic.",
	AC_NONE,
	2,
	cs_cmd_why
};

list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	command_add(&cs_why, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_why, cs_cmdtree);
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

	if (!chan || !targ)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "WHY");
		command_fail(si, fault_needmoreparams, "Syntax: WHY <channel> <user>");
		return;
	}

	mc = mychan_find(chan);
	u = user_find_named(targ);

	if (u == NULL)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not online.",
			targ);
		return;
	}

	mu = u->myuser;

	if (mc == NULL)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.",
			chan);
		return;
	}
	
	logcommand(chansvs.me, si->su, CMDLOG_GET, "%s WHY %s!%s@%s", mc->name, u->nick, u->user, u->vhost);

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.",
			targ);
		return;
	}

	snprintf(host, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->vhost);

	LIST_FOREACH(n, mc->chanacs.head)
	{
       	        ca = (chanacs_t *)n->data;

		if (ca->myuser != mu && match(ca->host, host))
			continue;

		if ((ca->level & CA_AUTOVOICE) == CA_AUTOVOICE)
			command_success_nodata(si,
				"\2%s\2 was granted voice in \2%s\2 because they have identified to the nickname %s which has the autovoice privilege.", 
				targ, chan, ca->myuser);

		if ((ca->level & CA_AUTOHALFOP) == CA_AUTOHALFOP)
			command_success_nodata(si,
				"\2%s\2 was granted halfops in \2%s\2 because they have identified to the nickname %s which has the autohalfop privilege.",
				targ, chan, ca->myuser);

		if ((ca->level & CA_AUTOOP) == CA_AUTOOP)
			command_success_nodata(si,
				"\2%s\2 was granted channel ops in \2%s\2 because they have identified to the nickname %s which has the autoop privilege.",
				targ, chan, ca->myuser);

		if ((ca->level & CA_AKICK) == CA_AKICK)
			command_success_nodata(si,
				"\2%s\2 is autokicked from \2%s\2 because they match the mask \2%s\2 that is on the channel akick list.",
				targ, chan, ca->host);

	}
}
