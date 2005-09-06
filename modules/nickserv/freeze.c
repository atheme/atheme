/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Gives services the ability to freeze nicknames
 *
 * $Id: freeze.c 2179 2005-09-06 09:17:45Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/freeze", FALSE, _modinit, _moddeinit,
	"$Id: freeze.c 2179 2005-09-06 09:17:45Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_freeze(char *origin);

/* FREEZE ON|OFF -- don't pollute the root with THAW */
command_t ns_freeze = { "FREEZE", "Freezes a nickname.",
			AC_IRCOP, ns_cmd_freeze };

list_t *ns_cmdtree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");

	command_add(&ns_freeze, ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_freeze, ns_cmdtree);
}

static void ns_cmd_freeze(char *origin)
{
	myuser_t *mu;
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *reason = strtok(NULL, "");

	if (!target || !action)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2FREEZE\2.");
		notice(nicksvs.nick, origin, "Usage: FREEZE <username> <ON|OFF> [reason]");
		return;
	}

	mu = myuser_find(target);

	if (!mu)
        {
                notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
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
                notice(nicksvs.nick, origin, "%s is a services root administrator and cannot be frozen.", target);
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
		notice(nicksvs.nick, origin, "\2%s\2 has been thawed", target);
	}
	else
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2FREEZE\2.");
		notice(nicksvs.nick, origin, "Usage: FREEZE <account> <ON|OFF> [reason]");
	}
}
