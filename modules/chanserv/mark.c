/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for channels.
 *
 * $Id: mark.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/mark", FALSE, _modinit, _moddeinit,
	"$Id: mark.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_mark(char *origin);

command_t cs_mark = { "MARK", "Adds a note to a channel.",
			PRIV_MARK, cs_cmd_mark };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_mark, cs_cmdtree);
	help_addentry(cs_helptree, "MARK", "help/cservice/mark", NULL);
}

void _moddeinit()
{
	command_delete(&cs_mark, cs_cmdtree);
	help_delentry(cs_helptree, "MARK");
}

static void cs_cmd_mark(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *info = strtok(NULL, "");
	mychan_t *mc;

	if (!target || !action)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "MARK");
		notice(chansvs.nick, origin, "Usage: MARK <#channel> <ON|OFF> [note]");
		return;
	}

	if (target[0] != '#')
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "MARK");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}
	
	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "MARK");
			notice(chansvs.nick, origin, "Usage: MARK <#channel> ON <note>");
			return;
		}

		if (metadata_find(mc, METADATA_CHANNEL, "private:mark:setter"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is already marked.", target);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "private:mark:setter", origin);
		metadata_add(mc, METADATA_CHANNEL, "private:mark:reason", info);
		metadata_add(mc, METADATA_CHANNEL, "private:mark:timestamp", itoa(CURRTIME));

		wallops("%s marked the channel \2%s\2.", origin, target);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s MARK ON", mc->name);
		notice(chansvs.nick, origin, "\2%s\2 is now marked.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, METADATA_CHANNEL, "private:mark:setter"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not marked.", target);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "private:mark:setter");
		metadata_delete(mc, METADATA_CHANNEL, "private:mark:reason");
		metadata_delete(mc, METADATA_CHANNEL, "private:mark:timestamp");

		wallops("%s unmarked the channel \2%s\2.", origin, target);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s MARK OFF", mc->name);
		notice(chansvs.nick, origin, "\2%s\2 is now unmarked.", target);
	}
	else
	{
		notice(chansvs.nick, origin, STR_INVALID_PARAMS, "MARK");
		notice(chansvs.nick, origin, "Usage: MARK <#channel> <ON|OFF> [note]");
	}
}
