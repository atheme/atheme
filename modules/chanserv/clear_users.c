/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ CLEAR USERS function.
 *
 * $Id: clear_users.c 2991 2005-10-18 23:55:43Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_users", FALSE, _modinit, _moddeinit,
	"$Id: clear_users.c 2991 2005-10-18 23:55:43Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_users(char *origin, char *channel);

fcommand_t cs_clear_users = { "USERS", AC_NONE, cs_cmd_clear_users };

list_t *cs_clear_cmds;

void _modinit(module_t *m)
{
	cs_clear_cmds = module_locate_symbol("chanserv/clear", "cs_clear_cmds");
	fcommand_add(&cs_clear_users, cs_clear_cmds);
}

void _moddeinit()
{
	fcommand_delete(&cs_clear_users, cs_clear_cmds);
}

static void cs_cmd_clear_users(char *origin, char *channel)
{
	char *reason = strtok(NULL, "");
	char fullreason[200];
	user_t *u = user_find(origin);
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	chanacs_t *ca;
	chanuser_t *cu;
	node_t *n, *tn;

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

	mode_sts(chansvs.nick, c->name, "+b *!*@*");

	LIST_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;

		/* don't kick the user who requested the masskick */

		if (!irccasecmp(cu->user->nick, origin) || cu->user->server == me.me)
			continue;

		kick(chansvs.nick, c->name, cu->user->nick, fullreason);
		chanuser_delete(c, cu->user);
	}

	notice(chansvs.nick, origin, "Cleared users from \2%s\2.", channel);

	/* stop a race condition where users can rejoin */
	cmode(chansvs.nick, c->name, "-b", "*!*@*");
}
