/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for channels.
 *
 * $Id: hold.c 1983 2005-09-01 03:14:44Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1("chanserv/hold", FALSE, _modinit, _moddeinit);

static void cs_cmd_hold(char *origin);

command_t cs_hold = { "HOLD", "Prevents a channel from expiring.",
			AC_SRA, cs_cmd_hold };

list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	command_add(&cs_hold, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_hold, cs_cmdtree);
}

static void cs_cmd_hold(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	mychan_t *mc;

	if (!target || !action)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2HOLD\2.");
		notice(chansvs.nick, origin, "Usage: HOLD <#channel> <ON|OFF>");
		return;
	}

	if (*target != '#')
	{
		notice(chansvs.nick, origin, "Invalid parameters for \2HOLD\2.");
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
		notice(chansvs.nick, origin, "\2%s\2 is no longer held.", target);
	}
	else
	{
		notice(chansvs.nick, origin, "Invalid parameters for \2HOLD\2.");
		notice(chansvs.nick, origin, "Usage: HOLD <#channel> <ON|OFF>");
	}
}
