/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ CLEAR USERS function.
 *
 * $Id: clear_users.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_users", FALSE, _modinit, _moddeinit,
	"$Id: clear_users.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_users(char *origin, char *channel);

fcommand_t cs_clear_users = { "USERS", AC_NONE, cs_cmd_clear_users };

list_t *cs_clear_cmds;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_clear_cmds, "chanserv/clear", "cs_clear_cmds");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	fcommand_add(&cs_clear_users, cs_clear_cmds);
	help_addentry(cs_helptree, "CLEAR USERS", "help/cservice/clear_users", NULL);
}

void _moddeinit()
{
	fcommand_delete(&cs_clear_users, cs_clear_cmds);

	help_delentry(cs_helptree, "CLEAR USERS");
}

static void cs_cmd_clear_users(char *origin, char *channel)
{
	char *reason = strtok(NULL, "");
	char fullreason[200];
	user_t *u = user_find_named(origin);
	channel_t *c;
	mychan_t *mc = mychan_find(channel);
	chanuser_t *cu;
	node_t *n, *tn;
	int oldlimit;

	if (!(c = channel_find(channel)))
	{
		notice(chansvs.nick, origin, "\2%s\2 does not exist.", channel);
		return;
	}

	if (reason)
		snprintf(fullreason, sizeof fullreason, "CLEAR USERS used by %s: %s", origin, reason);
	else
		snprintf(fullreason, sizeof fullreason, "CLEAR USERS used by %s", origin);

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", c->name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_RECOVER))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
		return;
	}

	/* stop a race condition where users can rejoin */
	oldlimit = c->limit;
	if (oldlimit != 1)
		modestack_mode_limit(chansvs.nick, c->name, MTYPE_ADD, 1);
	modestack_flush_channel(c->name);

	LIST_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;

		/* don't kick the user who requested the masskick */
		if (cu->user == u || is_internal_client(cu->user))
			continue;

		kick(chansvs.nick, c->name, cu->user->nick, fullreason);
		chanuser_delete(c, cu->user);
	}

	/* the channel may be empty now, so our pointer may be bogus! */
	c = channel_find(channel);
	if (c != NULL)
	{
		if (config_options.join_chans && !config_options.leave_chans
				&& c != NULL && !chanuser_find(c, u))
		{
			/* Always cycle it if the requester is not on channel
			 * -- jilles */
			part(channel, chansvs.nick);
		}
		/* could be permanent channel, blah */
		c = channel_find(channel);
		if (c != NULL)
		{
			if (oldlimit == 0)
				modestack_mode_limit(chansvs.nick, c->name, MTYPE_DEL, 0);
			else if (oldlimit != 1)
				modestack_mode_limit(chansvs.nick, c->name, MTYPE_ADD, oldlimit);
		}
	}

	logcommand(chansvs.me, u, CMDLOG_DO, "%s CLEAR USERS", mc->name);

	notice(chansvs.nick, origin, "Cleared users from \2%s\2.", channel);
}
