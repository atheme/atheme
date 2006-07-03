/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Gives services the ability to freeze nicknames
 *
 * $Id: freeze.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/freeze", FALSE, _modinit, _moddeinit,
	"$Id: freeze.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_freeze(char *origin);

/* FREEZE ON|OFF -- don't pollute the root with THAW */
command_t ns_freeze = { "FREEZE", "Freezes a nickname.",
			PRIV_USER_ADMIN, ns_cmd_freeze };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_freeze, ns_cmdtree);
	help_addentry(ns_helptree, "FREEZE", "help/nickserv/freeze", NULL);
}

void _moddeinit()
{
	command_delete(&ns_freeze, ns_cmdtree);
	help_delentry(ns_helptree, "FREEZE");
}

static void ns_cmd_freeze(char *origin)
{
	myuser_t *mu;
	user_t *source = user_find_named(origin);
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *reason = strtok(NULL, "");

	if (source == NULL)
		return;

	if (!target || !action)
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FREEZE");
		notice(nicksvs.nick, origin, "Usage: FREEZE <username> <ON|OFF> [reason]");
		return;
	}

	mu = myuser_find_ext(target);

	if (!mu)
        {
                notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
                return;
        }

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FREEZE");
			notice(nicksvs.nick, origin, "Usage: FREEZE <username> ON <reason>");
			return;
		}

	if (is_soper(mu))
	{
                notice(nicksvs.nick, origin, "The nickname \2%s\2 belongs to a services operator; it cannot be frozen.", target);
		return;
	}


		if (metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is already frozen.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:freeze:freezer", origin);
		metadata_add(mu, METADATA_USER, "private:freeze:reason", reason);
		metadata_add(mu, METADATA_USER, "private:freeze:timestamp", itoa(CURRTIME));

		wallops("%s froze the nickname \2%s\2 (%s).", origin, target, reason);
		logcommand(nicksvs.me, source, CMDLOG_ADMIN, "FREEZE %s ON", target);
		notice(nicksvs.nick, origin, "\2%s\2 is now frozen.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not frozen.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:freeze:freezer");
		metadata_delete(mu, METADATA_USER, "private:freeze:reason");
		metadata_delete(mu, METADATA_USER, "private:freeze:timestamp");

		wallops("%s thawed the nickname \2%s\2.", origin, target);
		logcommand(nicksvs.me, source, CMDLOG_ADMIN, "FREEZE %s OFF", target);
		notice(nicksvs.nick, origin, "\2%s\2 has been thawed", target);
	}
	else
	{
		notice(nicksvs.nick, origin, STR_INSUFFICIENT_PARAMS, "FREEZE");
		notice(nicksvs.nick, origin, "Usage: FREEZE <account> <ON|OFF> [reason]");
	}
}
