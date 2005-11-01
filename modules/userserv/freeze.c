/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Gives services the ability to freeze accounts
 *
 * $Id: freeze.c 3371 2005-11-01 00:33:18Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/freeze", FALSE, _modinit, _moddeinit,
	"$Id: freeze.c 3371 2005-11-01 00:33:18Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_freeze(char *origin);

/* FREEZE ON|OFF -- don't pollute the root with THAW */
command_t us_freeze = { "FREEZE", "Freezes a account.",
			AC_IRCOP, us_cmd_freeze };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_freeze, us_cmdtree);
	help_addentry(us_helptree, "FREEZE", "help/userserv/freeze", NULL);
}

void _moddeinit()
{
	command_delete(&us_freeze, us_cmdtree);
	help_delentry(us_helptree, "FREEZE");
}

static void us_cmd_freeze(char *origin)
{
	myuser_t *mu;
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *reason = strtok(NULL, "");

	if (!target || !action)
	{
		notice(usersvs.nick, origin, "Insufficient parameters for \2FREEZE\2.");
		notice(usersvs.nick, origin, "Usage: FREEZE <username> <ON|OFF> [reason]");
		return;
	}

	mu = myuser_find(target);

	if (!mu)
        {
                notice(usersvs.nick, origin, "\2%s\2 is not a registered account.", target);
                return;
        }

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			notice(chansvs.nick, origin, "Insufficient parameters for \2FREEZE\2.");
			notice(chansvs.nick, origin, "Usage: FREEZE <username> ON <reason>");
			return;
		}

		if (is_sra(mu))
		{
	                notice(usersvs.nick, origin, "The nickname \2%s\2 belongs to a services root administrator; it cannot be frozen.", target);
			return;
		}

		if (metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			notice(usersvs.nick, origin, "\2%s\2 is already frozen.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:freeze:freezer", origin);
		metadata_add(mu, METADATA_USER, "private:freeze:reason", reason);
		metadata_add(mu, METADATA_USER, "private:freeze:timestamp", itoa(CURRTIME));

		wallops("%s froze the account \2%s\2 (%s).", origin, target, reason);
		notice(usersvs.nick, origin, "\2%s\2 is now frozen.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
		{
			notice(usersvs.nick, origin, "\2%s\2 is not frozen.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:freeze:freezer");
		metadata_delete(mu, METADATA_USER, "private:freeze:reason");
		metadata_delete(mu, METADATA_USER, "private:freeze:timestamp");

		wallops("%s thawed the account \2%s\2.", origin, target);
		notice(usersvs.nick, origin, "\2%s\2 has been thawed", target);
	}
	else
	{
		notice(usersvs.nick, origin, "Insufficient parameters for \2FREEZE\2.");
		notice(usersvs.nick, origin, "Usage: FREEZE <account> <ON|OFF> [reason]");
	}
}
