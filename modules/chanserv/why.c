/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ MYACCESS function.
 *
 * $Id: why.c 4523 2006-01-06 11:30:36Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/why", FALSE, _modinit, _moddeinit,
	"$Id: why.c 4523 2006-01-06 11:30:36Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_why(char *origin);

command_t cs_why = {
	"WHY",
	"Explains channel access logic.",
	AC_NONE,
	cs_cmd_why
};

list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	command_add(&cs_why, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_why, cs_cmdtree);
}

static void cs_cmd_why(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *targ = strtok(NULL, " ");
	char host[BUFSIZE];
	mychan_t *mc;
	user_t *u, *tu;
	user_t *ou = user_find(origin);
	myuser_t *mu;
	node_t *n;
	chanacs_t *ca;
	uint32_t i, matches = 0;
	int operoverride = 0;

	if (!chan || !targ)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "WHY");
		notice(chansvs.nick, origin, "Syntax: WHY <channel> <user>");
		return;
	}

	mc = mychan_find(chan);
	u = user_find_named(origin);
	tu = user_find_named(targ);

	if (tu == NULL)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not online.",
			targ);
		return;
	}

	mu = tu->myuser;

	if (mc == NULL)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.",
			chan);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_ACLVIEW))
	{
		if (has_priv(u, PRIV_CHAN_AUSPEX))
			operoverride = 1;

		else
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(u, PRIV_CHAN_AUSPEX))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	if (mu == NULL)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.",
			targ);
		return;
	}

	if (operoverride)
		logcommand(chansvs.me, ou, CMDLOG_ADMIN, "%s WHY %s!%s@%s", mc->name, u->nick, u->user, u->vhost);
	else
		logcommand(chansvs.me, ou, CMDLOG_GET, "%s WHY %s!%s@%s", mc->name, u->nick, u->user, u->vhost);


	snprintf(host, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->vhost);

	LIST_FOREACH(n, mc->chanacs.head)
	{
       	        ca = (chanacs_t *)n->data;

		if (ca->myuser != mu && match(ca->host, host))
			continue;

		if ((ca->level & CA_AUTOVOICE) == CA_AUTOVOICE)
			notice(chansvs.nick, origin,
				"\2%s\2 was granted voice in \2%s\2 because they have identified to the nickname %s which has the autovoice privilege", 
				targ, chan, ca->myuser);

		if ((ca->level & CA_AUTOHALFOP) == CA_AUTOHALFOP)
			notice(chansvs.nick, origin,
				"\2%s\2 was granted halfops in \2%s\2 because they have identified to the nickname %s which has the autohalfop privilege.",
				targ, chan, ca->myuser);

		if ((ca->level & CA_AUTOOP) == CA_AUTOOP)
			notice(chansvs.nick, origin,
				"\2%s\2 was granted channel ops in \2%s\2 because they have identified to the nickname %s which has the autoop privilege.",
				targ, chan, ca->myuser);

		if ((ca->level & CA_AKICK) == CA_AKICK)
			notice(chansvs.nick, origin,
				"\2%s\2 was kicked from \2%s\2 because they are on the channel autokick list.",
				targ, chan);
	}
}
