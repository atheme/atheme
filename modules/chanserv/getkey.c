/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService GETKEY functions.
 *
 * $Id: getkey.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/getkey", FALSE, _modinit, _moddeinit,
	"$Id: getkey.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_getkey(char *origin);

command_t cs_getkey = { "GETKEY", "Returns the key (+k) of a channel.",
                        AC_NONE, cs_cmd_getkey };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_getkey, cs_cmdtree);
	help_addentry(cs_helptree, "GETKEY", "help/cservice/getkey", NULL);
}

void _moddeinit()
{
	command_delete(&cs_getkey, cs_cmdtree);
	help_delentry(cs_helptree, "GETKEY");
}

static void cs_cmd_getkey(char *origin)
{
	char *chan = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u = user_find_named(origin);

	if (!chan)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "GETKEY");
		notice(chansvs.nick, origin, "Syntax: GETKEY <#channel>");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "Cannot GETKEY: \2%s\2 is closed.", chan);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_INVITE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (!mc->chan)
	{
		notice(chansvs.nick, origin, "\2%s\2 is currently empty.", mc->name);
		return;
	}

	if (!mc->chan->key)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not keyed.", mc->name);
		return;
	}
	logcommand(chansvs.me, u, CMDLOG_GET, "%s GETKEY", mc->name);
	notice(chansvs.nick, origin, "Channel \2%s\2 key is: %s",
			mc->name, mc->chan->key);
}
