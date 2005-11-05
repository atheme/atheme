/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INVITE functions.
 *
 * $Id: invite.c 3483 2005-11-05 09:45:42Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/invite", FALSE, _modinit, _moddeinit,
	"$Id: invite.c 3483 2005-11-05 09:45:42Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_invite(char *origin);

command_t cs_invite = { "INVITE", "Invites a user to a channel.",
                        AC_NONE, cs_cmd_invite };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

        command_add(&cs_invite, cs_cmdtree);
	help_addentry(cs_helptree, "INVITE", "help/cservice/invite", NULL);
}

void _moddeinit()
{
	command_delete(&cs_invite, cs_cmdtree);
	help_delentry(cs_helptree, "INVITE");
}

static void cs_cmd_invite(char *origin)
{
	char *chan = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u = user_find(origin);

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2INVITE\2.");
		notice(chansvs.nick, origin, "Syntax: INVITE <#channel>");
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

	if (!chanacs_user_has_flag(mc, u, CA_INVITE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (chanuser_find(mc->chan, u))
	{
		notice(chansvs.nick, origin, "You're already on \2%s\2.", mc->name);
		return;
	}

	sts(":%s INVITE %s :%s", chansvs.nick, u->nick, chan);
	notice(chansvs.nick, origin, "You have been invited to \2%s\2.", mc->name);
}
