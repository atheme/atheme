/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INVITE functions.
 *
 * $Id: invite.c 2319 2005-09-23 14:01:26Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/invite", FALSE, _modinit, _moddeinit,
	"$Id: invite.c 2319 2005-09-23 14:01:26Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_invite(char *origin);

command_t cs_invite = { "INVITE", "Invites a user to a channel.",
                        AC_NONE, cs_cmd_invite };
                                                                                   
list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
        command_add(&cs_invite, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_invite, cs_cmdtree);
}

static void cs_cmd_invite(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2INVITE\2.");
		notice(chansvs.nick, origin, "Syntax: INVITE <#channel> [nickname]");
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
		notice(chansvs.nick, origin, "Cannot INVITE: \2%s\2 is closed.", chan);
		return;
	}

	u = user_find(origin);
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!is_xop(mc, u->myuser, CA_INVITE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to invite */
	if (nick)
	{
		if (!(u = user_find_named(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}

	cu = chanuser_find(mc->chan, u);
	if (cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already on \2%s\2.", u->nick, mc->name);
		return;
	}

	sts(":%s INVITE %s :%s", chansvs.nick, u->nick, chan);
	notice(chansvs.nick, origin, "\2%s\2 has been invited to \2%s\2.", u->nick, mc->name);
}
