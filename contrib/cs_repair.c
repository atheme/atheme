/*
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file allows SRAs to fix channels.
 *
 * $Id: xop.c 4763 2006-02-04 17:15:15Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/repair", FALSE, _modinit, _moddeinit,
	"$Id: xop.c 4763 2006-02-04 17:15:15Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_repair(char *origin);
static void cs_help_repair(char *origin);

command_t cs_repair = { "REPAIR", "Repairs channels from a merged database.",
                         PRIV_CHAN_ADMIN, cs_cmd_repair };

list_t *cs_cmdtree, *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

	command_add(&cs_repair, cs_cmdtree);

	help_addentry(cs_helptree, "REPAIR", NULL, cs_help_repair);
}

void _moddeinit()
{
	command_delete(&cs_repair, cs_cmdtree);

	help_delentry(cs_helptree, "REPAIR");
}

static void cs_help_repair(char *origin)
{
	notice (chansvs.nick, origin, "Syntax - REPAIR <#channel>");
	notice (chansvs.nick, origin, " ");
	notice (chansvs.nick, origin, "Like FORCEXOP but for SRAs.");
}

static void cs_cmd_repair(char *origin)
{
	char *chan = strtok(NULL, " ");
	chanacs_t *ca;
	mychan_t *mc = mychan_find(chan);
	user_t *u = user_find_named(origin);
	node_t *n;
	int i, changes;
	uint32_t newlevel;
	char *desc;

	if (!chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FORCEXOP");
		notice(chansvs.nick, origin, "Syntax: FORCEXOP <#channel>");
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	changes = 0;
	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->level & CA_AKICK)
			continue;
		if (ca->myuser && is_founder(mc, ca->myuser))
			newlevel = CA_INITIAL, desc = "Founder";
		else if (!(~ca->level & chansvs.ca_sop))
			newlevel = chansvs.ca_sop, desc = "SOP";
		else if (ca->level == chansvs.ca_aop)
			newlevel = chansvs.ca_aop, desc = "AOP";
		else if (ca->level == chansvs.ca_hop)
			newlevel = chansvs.ca_hop, desc = "HOP";
		else if (ca->level == chansvs.ca_vop)
			newlevel = chansvs.ca_vop, desc = "VOP";
		else if (ca->level & (CA_SET | CA_RECOVER | CA_FLAGS))
			newlevel = chansvs.ca_sop, desc = "SOP";
		else if (ca->level & (CA_OP | CA_AUTOOP | CA_REMOVE))
			newlevel = chansvs.ca_aop, desc = "AOP";
		else if (ca->level & (CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC))
			newlevel = chansvs.ca_hop, desc = "HOP";
		else /*if (ca->level & CA_AUTOVOICE)*/
			newlevel = chansvs.ca_vop, desc = "VOP";
#if 0
		else
			newlevel = 0;
#endif
		if (newlevel == ca->level)
			continue;
		changes++;
		notice(chansvs.nick, origin, "%s: %s -> %s", ca->myuser ? ca->myuser->name : ca->host, bitmask_to_flags(ca->level, chanacs_flags), desc);
		ca->level = newlevel;
	}
	notice(chansvs.nick, origin, "REPAIR \2%s\2 done (\2%d\2 changes)", mc->name, changes);
	if (changes > 0)
		verbose(mc, "\2%s\2 reset access levels to xOP (\2%d\2 changes)", u->nick, changes);
	wallops("\2%s\2 is repairing \2%s\2!", u->nick, mc->name);
	logcommand(chansvs.me, u, CMDLOG_SET, "%s REPAIR (%d changes)", mc->name, changes);
}
