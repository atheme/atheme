/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for accounts.
 *
 * $Id: hold.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/hold", FALSE, _modinit, _moddeinit,
	"$Id: hold.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_hold(char *origin);

command_t us_hold = { "HOLD", "Prevents a account from expiring.",
			PRIV_HOLD, us_cmd_hold };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_hold, us_cmdtree);
	help_addentry(us_helptree, "HOLD", "help/userserv/hold", NULL);
}

void _moddeinit()
{
	command_delete(&us_hold, us_cmdtree);
	help_delentry(us_helptree, "HOLD");
}

static void us_cmd_hold(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	myuser_t *mu;
	user_t *source = user_find_named(origin);

	if (source == NULL)
		return;

	if (!target || !action)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "HOLD");
		notice(usersvs.nick, origin, "Usage: HOLD <account> <ON|OFF>");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
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
		logcommand(usersvs.me, source, CMDLOG_ADMIN, "HOLD %s ON", target);
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
		logcommand(usersvs.me, source, CMDLOG_ADMIN, "HOLD %s OFF", target);
		notice(usersvs.nick, origin, "\2%s\2 is no longer held.", target);
	}
	else
	{
		notice(usersvs.nick, origin, STR_INVALID_PARAMS, "HOLD");
		notice(usersvs.nick, origin, "Usage: HOLD <account> <ON|OFF>");
	}
}
