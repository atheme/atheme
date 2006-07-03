/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for channels.
 *
 * $Id: hold.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/hold", FALSE, _modinit, _moddeinit,
	"$Id: hold.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_hold(char *origin);

command_t cs_hold = { "HOLD", "Prevents a channel from expiring.",
			PRIV_HOLD, cs_cmd_hold };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_hold, cs_cmdtree);
	help_addentry(cs_helptree, "HOLD", "help/cservice/hold", NULL);
}

void _moddeinit()
{
	command_delete(&cs_hold, cs_cmdtree);
	help_delentry(cs_helptree, "HOLD");
}

static void cs_cmd_hold(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	mychan_t *mc;

	if (!target || !action)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "HOLD");
		notice(chansvs.nick, origin, "Usage: HOLD <#channel> <ON|OFF>");
		return;
	}

	if (*target != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "HOLD");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}
	
	if (!strcasecmp(action, "ON"))
	{
		if (mc->flags & MC_HOLD)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already held.", target);
			return;
		}

		mc->flags |= MC_HOLD;

		wallops("%s set the HOLD option for the channel \2%s\2.", origin, target);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s HOLD ON", mc->name);
		notice(chansvs.nick, origin, "\2%s\2 is now held.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mc->flags & MC_HOLD))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not held.", target);
			return;
		}

		mc->flags &= ~MC_HOLD;

		wallops("%s removed the HOLD option on the channel \2%s\2.", origin, target);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s HOLD OFF", mc->name);
		notice(chansvs.nick, origin, "\2%s\2 is no longer held.", target);
	}
	else
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "HOLD");
		notice(chansvs.nick, origin, "Usage: HOLD <#channel> <ON|OFF>");
	}
}
