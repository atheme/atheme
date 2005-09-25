/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for accounts.
 *
 * $Id: hold.c 2361 2005-09-25 03:05:34Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/hold", FALSE, _modinit, _moddeinit,
	"$Id: hold.c 2361 2005-09-25 03:05:34Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_hold(char *origin);

command_t us_hold = { "HOLD", "Prevents a account from expiring.",
			AC_SRA, us_cmd_hold };

list_t *us_cmdtree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	command_add(&us_hold, us_cmdtree);
}

void _moddeinit()
{
	command_delete(&us_hold, us_cmdtree);
}

static void us_cmd_hold(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	myuser_t *mu;

	if (!target || !action)
	{
		notice(usersvs.nick, origin, "Insufficient parameters for \2HOLD\2.");
		notice(usersvs.nick, origin, "Usage: HOLD <account> <ON|OFF>");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (mu->flags & MU_HOLD)
		{
			notice(usersvs.nick, origin, "\2%s\2 is already held.", target);
			return;
		}

		mu->flags |= MU_HOLD;

		wallops("%s set the HOLD option for the account \2%s\2.", origin, target);
		notice(usersvs.nick, origin, "\2%s\2 is now held.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mu->flags & MU_HOLD))
		{
			notice(usersvs.nick, origin, "\2%s\2 is not held.", target);
			return;
		}

		mu->flags &= ~MU_HOLD;

		wallops("%s removed the HOLD option on the account \2%s\2.", origin, target);
		notice(usersvs.nick, origin, "\2%s\2 is no longer held.", target);
	}
	else
	{
		notice(usersvs.nick, origin, "Invalid parameters for \2HOLD\2.");
		notice(usersvs.nick, origin, "Usage: HOLD <account> <ON|OFF>");
	}
}
