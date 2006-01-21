/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: clear_bans.c 4639 2006-01-21 22:06:41Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_bans", FALSE, _modinit, _moddeinit,
	"$Id: clear_bans.c 4639 2006-01-21 22:06:41Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_bans(char *origin, char *channel);

fcommand_t cs_clear_bans = { "BANS", AC_NONE, cs_cmd_clear_bans };

list_t *cs_clear_cmds;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_clear_cmds = module_locate_symbol("chanserv/clear", "cs_clear_cmds");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

	fcommand_add(&cs_clear_bans, cs_clear_cmds);
	help_addentry(cs_helptree, "CLEAR BANS", "help/cservice/clear_bans", NULL);	
}

void _moddeinit()
{
	fcommand_delete(&cs_clear_bans, cs_clear_cmds);

	help_delentry(cs_helptree, "CLEAR BANS");
}

static void cs_cmd_clear_bans(char *origin, char *channel)
{
	user_t *u = user_find_named(origin);
	channel_t *c;
	mychan_t *mc = mychan_find(channel);
	chanban_t *cb;
	node_t *n, *tn;

	if (!(c = channel_find(channel)))
	{
		notice(chansvs.nick, origin, "\2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", c->name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_FLAGS))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
		return;
	}

	LIST_FOREACH_SAFE(n, tn, c->bans.head)
	{
		cb = n->data;
		if (cb->type != 'b')
			continue;
		cmode(chansvs.nick, c->name, "-b", cb->mask);
		chanban_delete(cb);
	}

	logcommand(chansvs.me, u, CMDLOG_DO, "%s CLEAR BANS", mc->name);

	notice(chansvs.nick, origin, "Cleared bans on \2%s\2.", channel);
}
