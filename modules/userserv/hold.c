/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for nicknames.
 *
 * $Id: hold.c 2049 2005-09-02 05:45:25Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("nickserv/hold", FALSE, _modinit, _moddeinit);

static void ns_cmd_hold(char *origin);

command_t ns_hold = { "HOLD", "Prevents a nickname from expiring.",
			AC_SRA, ns_cmd_hold };

list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	command_add(&ns_hold, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_hold, ns_cmdtree);
}

static void ns_cmd_hold(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	myuser_t *mu;

	if (!target || !action)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2HOLD\2.");
		notice(nicksvs.nick, origin, "Usage: HOLD <nickname> <ON|OFF>");
		return;
	}

	if (!(mu = myuser_find(target)))
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
		notice(nicksvs.nick, origin, "\2%s\2 is no longer held.", target);
	}
	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters for \2HOLD\2.");
		notice(nicksvs.nick, origin, "Usage: HOLD <nickname> <ON|OFF>");
	}
}
