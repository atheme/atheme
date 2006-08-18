/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * Regexp-based AKILL implementation.
 *
 * $Id$
 */

/*
 * @makill regex!here.+ [akill reason]
 *  Matches `nick!user@host realname here' for each client against a given regex, and places akills.
 *  CHECK REGEX USING @RMATCH FIRST!
 */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rakill", FALSE, _modinit, _moddeinit,
	"$Id$",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *os_cmdtree;

static void _os_rakill(char *origin);

command_t os_rakill = { "RAKILL", "Sets a group of AKILLs against users matching a specific regex pattern.", PRIV_ADMIN, _os_rakill };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");

	command_add(&os_rakill, os_cmdtree);
}

void _moddeinit(void)
{
	command_delete(&os_rakill, os_cmdtree);
}

static void _os_rakill(char *origin)
{
	regex_t *regex;
	char usermask[512];
	uint32_t matches = 0;
	uint32_t i = 0;
	node_t *n;
	user_t *u;
	char *pattern = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	user_t *me2 = user_find(origin);
	int flags = 0;

	if (pattern == NULL)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "RAKILL");
		notice(opersvs.nick, origin, "Syntax: RAKILL [!i:]<regex> [reason]");
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
			u = (user_t *) n->data;

			sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

			if (regex_match(regex, (char *)usermask) == TRUE)
			{
				/* match */
				notice(opersvs.nick, origin, "\2Match:\2  %s!%s@%s %s - akilling", u->nick, u->user, u->host, u->gecos);
				kline_sts(me.name, "*", u->host, 604800, reason ? reason : "(none)");
				matches++;
			}
		}
	}
	
	regex_destroy(regex);
	notice(opersvs.nick, origin, "\2%d\2 matches for %s akilled!", matches, pattern);
}
