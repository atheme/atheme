/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Gives services the ability to freeze accounts
 *
 * $Id: freeze.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/freeze", FALSE, _modinit, _moddeinit,
	"$Id: freeze.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_freeze(sourceinfo_t *si, int parc, char *parv[]);

/* FREEZE ON|OFF -- don't pollute the root with THAW */
command_t us_freeze = { "FREEZE", "Freezes a account.", PRIV_USER_ADMIN, 3, us_cmd_freeze };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	command_add(&us_freeze, us_cmdtree);
	help_addentry(us_helptree, "FREEZE", "help/userserv/freeze", NULL);
}

void _moddeinit()
{
	command_delete(&us_freeze, us_cmdtree);
	help_delentry(us_helptree, "FREEZE");
}

static void us_cmd_freeze(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *target = parv[0];
	char *action = parv[1];
	char *reason = parv[2];

	if (!target || !action)
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "FREEZE");
		notice(usersvs.nick, si->su->nick, "Usage: FREEZE <username> <ON|OFF> [reason]");
		return;
	}

	mu = myuser_find_ext(target);

	if (!mu)
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not a registered account.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "FREEZE");
			notice(usersvs.nick, si->su->nick, "Usage: FREEZE <username> ON <reason>");
			return;
		}

		if (is_soper(mu))
		{
			notice(usersvs.nick, si->su->nick, "The account \2%s\2 belongs to a services operator; it cannot be frozen.", target);
			return;
		}

		if (metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			notice(usersvs.nick, si->su->nick, "\2%s\2 is already frozen.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:freeze:freezer", si->su->nick);
		metadata_add(mu, METADATA_USER, "private:freeze:reason", reason);
		metadata_add(mu, METADATA_USER, "private:freeze:timestamp", itoa(CURRTIME));

		wallops("%s froze the account \2%s\2 (%s).", si->su->nick, target, reason);
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "FREEZE %s ON", target);
		notice(usersvs.nick, si->su->nick, "\2%s\2 is now frozen.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			notice(usersvs.nick, si->su->nick, "\2%s\2 is not frozen.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:freeze:freezer");
		metadata_delete(mu, METADATA_USER, "private:freeze:reason");
		metadata_delete(mu, METADATA_USER, "private:freeze:timestamp");

		wallops("%s thawed the account \2%s\2.", si->su->nick, target);
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "FREEZE %s OFF", target);
		notice(usersvs.nick, si->su->nick, "\2%s\2 has been thawed", target);
	}
	else
	{
		notice(usersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "FREEZE");
		notice(usersvs.nick, si->su->nick, "Usage: FREEZE <account> <ON|OFF> [reason]");
	}
}
