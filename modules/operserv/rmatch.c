/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Regex usersearch feature.
 *
 * $Id: rmatch.c 6123 2006-08-18 15:55:06Z jilles $
 */

/*
 * @match regex!here.+
 *  Matches `nick!user@host realname here' for each client against a given regex, and dumps matches.
 */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rmatch", FALSE, _modinit, _moddeinit,
	"$Id: rmatch.c 6123 2006-08-18 15:55:06Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *os_cmdtree;

static void os_cmd_rmatch(char *origin);

command_t os_rmatch = { "RMATCH", "Scans the network for users based on a specific regex pattern.", PRIV_USER_AUSPEX, os_cmd_rmatch };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");

	command_add(&os_rmatch, os_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&os_rmatch, os_cmdtree);
}

static void os_cmd_rmatch(char *origin)
{
	regex_t *regex;
	char usermask[512];
	uint32_t matches = 0;
	uint32_t i = 0;
	node_t *n;
	user_t *u;
	char *pattern = strtok(NULL, "");
	int flags = 0;

	if (pattern == NULL)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "RMATCH");
		notice(opersvs.nick, origin, "Syntax: RMATCH [!i:]<regex>");
		return;
	}

	/* insensitivity option -nenolod */
	if (!strncasecmp(pattern, "!i:", 3))
	{
		pattern += 3;
		flags = AREGEX_ICASE;
	}

	regex = regex_create(pattern, flags);
	
	if (regex == NULL)
	{
		notice(opersvs.nick, origin, "The provided regex is invalid.");
		return;
	}
		
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, userlist[i].head)
		{
			u = n->data;
			
			sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

			if (regex_match(regex, usermask) == TRUE)
			{
				/* match */
				notice(opersvs.nick, origin, "\2Match:\2  %s!%s@%s %s", u->nick, u->user, u->host, u->gecos);
				matches++;
			}
		}
	}
	
	regex_destroy(regex);
	notice(opersvs.nick, origin, "\2%d\2 matches for %s", matches, pattern);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "RMATCH %s (%d matches)", pattern, matches);
	snoop("RMATCH: \2%s\2 by \2%s\2", pattern, origin);
}
