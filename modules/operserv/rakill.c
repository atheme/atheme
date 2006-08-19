/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * Regexp-based AKILL implementation.
 *
 * $Id: rakill.c 6157 2006-08-19 22:05:04Z jilles $
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
	"$Id: rakill.c 6157 2006-08-19 22:05:04Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *os_cmdtree;
list_t *os_helptree;

static void os_cmd_rakill(char *origin);

command_t os_rakill = { "RAKILL", "Sets a group of AKILLs against users matching a specific regex pattern.", PRIV_MASS_AKILL, os_cmd_rakill };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_rakill, os_cmdtree);
	help_addentry(os_helptree, "RAKILL", "help/oservice/rakill", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_rakill, os_cmdtree);
	help_delentry(os_helptree, "RAKILL");
}

static void os_cmd_rakill(char *origin)
{
	regex_t *regex;
	char usermask[512];
	uint32_t matches = 0;
	uint32_t i = 0;
	node_t *n;
	user_t *u;
	char *pattern = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	user_t *me2 = user_find_named(origin);
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

	snoop("RAKILL: \2%s\2 by \2%s\2 (%s)", pattern, origin, reason ? reason : "(none)");

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
				kline_sts("*", "*", u->host, 604800, reason ? reason : "(none)");
				matches++;
			}
		}
	}
	
	regex_destroy(regex);
	notice(opersvs.nick, origin, "\2%d\2 matches for %s akilled!", matches, pattern);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "RAKILL %s: %s (%d matches)", pattern, reason ? reason : "(none)", matches);
}
