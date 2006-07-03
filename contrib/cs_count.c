/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService COUNT functions.
 *
 * $Id: cs_count.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/count", FALSE, _modinit, _moddeinit,
	"$Id: cs_count.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_count(char *origin);

command_t cs_count = { "COUNT", "Shows number of entries in xOP lists.",
                         AC_NONE, cs_cmd_count };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");
	help_addentry(cs_helptree, "COUNT", "help/cservice/count", NULL);
        command_add(&cs_count, cs_cmdtree);
}

void _moddeinit()
{
	help_delentry(cs_helptree, "COUNT");
	command_delete(&cs_count, cs_cmdtree);
}

static void cs_cmd_count(char *origin)
{
	char *chan = strtok(NULL, " ");
	chanacs_t *ca;
	mychan_t *mc = mychan_find(chan);
	user_t *u = user_find_named(origin);
	uint8_t vopcnt = 0, aopcnt = 0, hopcnt = 0, sopcnt = 0, akickcnt = 0;
	uint8_t othercnt = 0;
	int i;
	node_t *n;
	char str[512];

	if (!chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "COUNT");
		notice(chansvs.nick, origin, "Syntax: COUNT <#channel>");
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_ACLVIEW))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->level == chansvs.ca_vop)
			vopcnt++;
		else if (ca->level == chansvs.ca_hop)
			hopcnt++;
		else if (ca->level == chansvs.ca_aop)
			aopcnt++;
		else if (ca->level == chansvs.ca_sop)
			sopcnt++;
		else if (ca->level == CA_AKICK)
			akickcnt++;
		else if (ca->myuser != mc->founder)
			othercnt++;
	}
	notice(chansvs.nick, origin, "%s: VOp: %d, HOp: %d, AOp: %d, SOp: %d, AKick: %d, other: %d",
			chan, vopcnt, hopcnt, aopcnt, sopcnt, akickcnt, othercnt);
	snprintf(str, sizeof str, "%s: ", chan);
	for (i = 0; chanacs_flags[i].flag; i++)
	{
		othercnt = 0;
		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			if (ca->level & chanacs_flags[i].value)
				othercnt++;
		}
		snprintf(str + strlen(str), sizeof str - strlen(str),
				"%c:%d ", chanacs_flags[i].flag, othercnt);
	}
	notice(chansvs.nick, origin, "%s", str);
}

