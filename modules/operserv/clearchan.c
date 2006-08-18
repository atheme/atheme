/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ CLEARCHAN.
 *
 * $Id: clearchan.c 6127 2006-08-18 16:59:55Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/clearchan", FALSE, _modinit, _moddeinit,
	"$Id: clearchan.c 6127 2006-08-18 16:59:55Z jilles $",
	"Robin Burchell <surreal.w00t@gmail.com>"
);

#define CLEAR_KICK 1
#define CLEAR_KILL 2
#define CLEAR_GLINE 3

static void os_cmd_clearchan(char *origin);

command_t os_clearchan = { "CLEARCHAN", "Clears a channel via KICK, KILL or GLINE", PRIV_CHAN_ADMIN, os_cmd_clearchan };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_clearchan, os_cmdtree);
	help_addentry(os_helptree, "CLEARCHAN", "help/oservice/clearchan", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_clearchan, os_cmdtree);
	help_delentry(os_helptree, "CLEARCHAN");
}

static void os_cmd_clearchan(char *origin)
{
	chanuser_t *cu = NULL;
	node_t *n = NULL;
	channel_t *c = NULL;
	char *targchan = strtok(NULL, " ");
	char *actionstr = "KICK";
	int action = CLEAR_KICK;
	char *token = strtok(NULL, " ");
	char reason[BUFSIZE], *treason;
	int matches = 0;
	int ignores = 0;

	if (!targchan || !token)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLEARCHAN");
		notice(opersvs.nick, origin, "Syntax: CLEARCHAN <#channel> [!KICK|!KILL|!GLINE] <reason here>");
 		return;
	}

	if (*token == '!')
	{
		/* an action was specified */
		actionstr = token + 1;
		treason = strtok(NULL, "");

		if (treason)
		{
			strlcpy(reason, treason, BUFSIZE);
		}
		else
		{
			snprintf(reason, sizeof reason, "Clearing %s", targchan);
		}
	}
	else
	{
		strlcpy(reason, token, BUFSIZE);
		treason = strtok(NULL, "");

		if (treason)
		{
			strlcat(reason, " ", BUFSIZE);
			strlcat(reason, treason, BUFSIZE);
		}
	}

	c = channel_find(targchan);

	if (!c)
	{
		notice(opersvs.nick, origin, "The channel must exist for CLEARCHAN.");
 		return;				
	}

	/* check what they are doing is valid */
	if (!strcasecmp(actionstr, "KICK"))
		action = CLEAR_KICK;
	else if (!strcasecmp(actionstr, "KILL"))
		action = CLEAR_KILL;
	else if (!strcasecmp(actionstr, "GLINE") || !strcasecmp(actionstr, "KLINE"))
		action = CLEAR_GLINE;
	else
	{
		/* not valid! */
		notice(opersvs.nick, origin, "\2%s\2 is not a valid action", actionstr);
 		return;				
	}

	if (action != CLEAR_KICK && !has_priv(user_find_named(origin), PRIV_MASS_AKILL))
	{
		notice(opersvs.nick, origin, "You do not have %s privilege.", PRIV_MASS_AKILL);
		return;
	}

	wallops("\2%s\2 is clearing channel %s with %s",
			origin, targchan, actionstr);
	snoop("CLEARCHAN: %s on \2%s\2 by \2%s\2", actionstr, targchan, origin);
	notice(opersvs.nick, origin, "Clearing \2%s\2 with \2%s\2", targchan, actionstr);

	/* iterate over the users in channel */
	LIST_FOREACH(n, c->members.head)
	{
		cu = n->data;

		if (is_internal_client(cu->user))
			;
		else if (is_ircop(cu->user))
		{
			notice(opersvs.nick, origin, "\2CLEARCHAN\2: Ignoring IRC Operator \2%s\2!%s@%s {%s}", cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			ignores++;
		}
		else
		{
			notice(opersvs.nick, origin, "\2CLEARCHAN\2: \2%s\2 hit \2%s\2!%s@%s {%s}", actionstr, cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			matches++;

			switch (action)
			{
				case CLEAR_KICK:
					kick(opersvs.nick, targchan, cu->user->nick, reason);
					break;
				case CLEAR_KILL:
					skill(opersvs.nick, cu->user->nick, reason);
					user_delete(cu->user);
					break;
				case CLEAR_GLINE:
					kline_sts("*", "*", cu->user->host, 604800, reason);
			}
		}
	}

	notice(opersvs.nick, origin, "\2%d\2 matches, \2%d\2 ignores for \2%s\2 on \2%s\2", matches, ignores, actionstr, targchan);
	logcommand(opersvs.me, user_find_named(origin), CMDLOG_ADMIN, "CLEARCHAN %s %s (%d matches, %d ignores)", actionstr, targchan, matches, ignores);
}
