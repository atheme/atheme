/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService COUNT functions.
 *
 * $Id: count.c 2971 2005-10-17 09:52:19Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/count", FALSE, _modinit, _moddeinit,
	"$Id: count.c 2971 2005-10-17 09:52:19Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_count(char *origin);

command_t cs_count = { "COUNT", "Show number of entries in room lists.",
                         AC_NONE, cs_cmd_count };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");
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
	user_t *u = user_find(origin);
	uint8_t vopcnt = 0, aopcnt = 0, hopcnt = 0, sopcnt = 0;
	node_t *n;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2COUNT\2.");
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

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;
		if (ca->level == CA_VOP)
			vopcnt++;
		if (ca->level == CA_HOP)
			hopcnt++;
		if (ca->level == CA_AOP)
			aopcnt++;
		if (ca->level == CA_SOP)
			sopcnt++;
	}
	notice(chansvs.nick, origin, "%s: VOp: %d, HOp: %d, AOp: %d, SOp: %d", chan, vopcnt, hopcnt, aopcnt, sopcnt);

}

