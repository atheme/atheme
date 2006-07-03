/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for accounts.
 *
 * $Id: mark.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/mark", FALSE, _modinit, _moddeinit,
	"$Id: mark.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_mark(char *origin);

command_t us_mark = { "MARK", "Adds a note to a user.",
			PRIV_MARK, us_cmd_mark };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_mark, us_cmdtree);
	help_addentry(us_helptree, "MARK", "help/userserv/mark", NULL);
}

void _moddeinit()
{
	command_delete(&us_mark, us_cmdtree);
	help_delentry(us_helptree, "MARK");
}

static void us_cmd_mark(char *origin)
{
	user_t *source = user_find_named(origin);
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *info = strtok(NULL, "");
	myuser_t *mu;

	if (source == NULL)
		return;

	if (!target || !action)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "MARK");
		notice(usersvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "MARK");
			notice(usersvs.nick, origin, "Usage: MARK <target> ON <note>");
			return;
		}

		if (metadata_find(mu, METADATA_USER, "private:mark:setter"))
		{
			notice(usersvs.nick, origin, "\2%s\2 is already marked.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:mark:setter", origin);
		metadata_add(mu, METADATA_USER, "private:mark:reason", info);
		metadata_add(mu, METADATA_USER, "private:mark:timestamp", itoa(time(NULL)));

		wallops("%s marked the account \2%s\2.", origin, target);
		logcommand(usersvs.me, source, CMDLOG_ADMIN, "MARK %s ON (reason: %s)", target, info);
		notice(usersvs.nick, origin, "\2%s\2 is now marked.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:mark:setter"))
		{
			notice(usersvs.nick, origin, "\2%s\2 is not marked.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:mark:setter");
		metadata_delete(mu, METADATA_USER, "private:mark:reason");
		metadata_delete(mu, METADATA_USER, "private:mark:timestamp");

		wallops("%s unmarked the account \2%s\2.", origin, target);
		logcommand(usersvs.me, source, CMDLOG_ADMIN, "MARK %s OFF", target);
		notice(usersvs.nick, origin, "\2%s\2 is now unmarked.", target);
	}
	else
	{
		notice(usersvs.nick, origin, STR_INVALID_PARAMS, "MARK");
		notice(usersvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
	}
}
