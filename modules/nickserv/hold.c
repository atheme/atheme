/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for nicknames.
 *
 * $Id: hold.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/hold", FALSE, _modinit, _moddeinit,
	"$Id: hold.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_hold(char *origin);

command_t ns_hold = { "HOLD", "Prevents a nickname from expiring.",
			PRIV_HOLD, ns_cmd_hold };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_hold, ns_cmdtree);
	help_addentry(ns_helptree, "HOLD", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&ns_hold, ns_cmdtree);
	help_delentry(ns_helptree, "HOLD");
}

static void ns_cmd_hold(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	myuser_t *mu;
	user_t *source = user_find_named(origin);

	if (source == NULL)
		return;

	if (!target || !action)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "HOLD");
		notice(nicksvs.nick, origin, "Usage: HOLD <nickname> <ON|OFF>");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (mu->flags & MU_HOLD)
		{
			notice(nicksvs.nick, origin, "\2%s\2 is already held.", target);
			return;
		}

		mu->flags |= MU_HOLD;

		wallops("%s set the HOLD option for the nickname \2%s\2.", origin, target);
		logcommand(nicksvs.me, source, CMDLOG_ADMIN, "HOLD %s ON", target);
		notice(nicksvs.nick, origin, "\2%s\2 is now held.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mu->flags & MU_HOLD))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not held.", target);
			return;
		}

		mu->flags &= ~MU_HOLD;

		wallops("%s removed the HOLD option on the nickname \2%s\2.", origin, target);
		logcommand(nicksvs.me, source, CMDLOG_ADMIN, "HOLD %s OFF", target);
		notice(nicksvs.nick, origin, "\2%s\2 is no longer held.", target);
	}
	else
	{
		notice(nicksvs.nick, origin, STR_INVALID_PARAMS, "HOLD");
		notice(nicksvs.nick, origin, "Usage: HOLD <nickname> <ON|OFF>");
	}
}
