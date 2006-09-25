/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for nicknames.
 *
 * $Id: mark.c 6457 2006-09-25 10:33:40Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/mark", FALSE, _modinit, _moddeinit,
	"$Id: mark.c 6457 2006-09-25 10:33:40Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_mark(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_mark = { "MARK", "Adds a note to a user.", PRIV_MARK, 3, ns_cmd_mark };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_mark, ns_cmdtree);
	help_addentry(ns_helptree, "MARK", "help/nickserv/mark", NULL);
}

void _moddeinit()
{
	command_delete(&ns_mark, ns_cmdtree);
	help_delentry(ns_helptree, "MARK");
}

static void ns_cmd_mark(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	myuser_t *mu;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, "Usage: MARK <target> <ON|OFF> [note]");
		return;
	}

	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, "Usage: MARK <target> ON <note>");
			return;
		}

		if (metadata_find(mu, METADATA_USER, "private:mark:setter"))
		{
			command_fail(si, fault_badparams, "\2%s\2 is already marked.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:mark:setter", si->su->nick);
		metadata_add(mu, METADATA_USER, "private:mark:reason", info);
		metadata_add(mu, METADATA_USER, "private:mark:timestamp", itoa(time(NULL)));

		wallops("%s marked the nickname \2%s\2.", si->su->nick, target);
		logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "MARK %s ON (reason: %s)", target, info);
		command_success_nodata(si, "\2%s\2 is now marked.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:mark:setter"))
		{
			command_fail(si, fault_badparams, "\2%s\2 is not marked.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:mark:setter");
		metadata_delete(mu, METADATA_USER, "private:mark:reason");
		metadata_delete(mu, METADATA_USER, "private:mark:timestamp");

		wallops("%s unmarked the nickname \2%s\2.", si->su->nick, target);
		logcommand(nicksvs.me, si->su, CMDLOG_ADMIN, "MARK %s OFF", target);
		command_success_nodata(si, "\2%s\2 is now unmarked.", target);
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, "Usage: MARK <target> <ON|OFF> [note]");
	}
}
