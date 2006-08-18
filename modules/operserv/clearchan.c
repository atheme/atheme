/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ CLEARCHAN.
 *
 * $Id: clearchan.c 6107 2006-08-18 10:46:09Z w00t $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/clearchan", FALSE, _modinit, _moddeinit,
	"$Id: clearchan.c 6107 2006-08-18 10:46:09Z w00t $",
	"Robin Burchell <surreal.w00t@gmail.com>"
);

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
	char *action = "KICK";
	char *token = strtok(NULL, " ");
	char reason[BUFSIZE], *treason;
	int matches = 0;
	int ignores = 0;
	user_t *me2 = user_find(origin);

	if (!targchan || !token)
	{
		notice(opersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLEARCHAN");
		notice(opersvs.nick, origin, "Syntax: CLEARCHAN <#channel> [!KICK|!KILL|!GLINE] <reason here>");
 		return;
	}

	if (*token == '!')
	{
		/* an action was specified */
		action = token;
		action++; /* remove '!' */
		treason = strtok(NULL, "");

		if (treason)
		{
			strlcpy(reason, treason, BUFSIZE);
		}
		else
		{
			strlcpy(reason, "No reason provided", BUFSIZE);
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
	if (strcasecmp(action, "KICK") == 0)
	{
		/* valid, ignore */
	}
	else if (strcasecmp(action, "KILL") == 0)
	{
		/* valid, ignore */
	}
	else if (strcasecmp(action, "GLINE") == 0)
	{
		/* valid, ignore */
	}
	else
	{
		/* not valid! */
		notice(opersvs.nick, origin, "\2%s\2 is not a valid action", action);
 		return;				
	}

	notice(opersvs.nick, origin, "Clearing \2%s\2 by \2%s\2", targchan, action);

	/* iterate over the users in channel */
	LIST_FOREACH(n, c->members.head)
	{
		cu = n->data;

		if (is_ircop(cu->user))
		{
			notice(opersvs.nick, origin, "\2CLEARCHAN\2: Ignoring IRC Operator \2%s\2!%s@%s {%s}", cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			ignores++;
		}
		else
		{
			if (strcasecmp(action, "KICK") == 0)
			{
				kick(opersvs.nick, targchan, cu->user->nick, reason);
			}
			else if (strcasecmp(action, "KILL") == 0)
			{
				skill(opersvs.nick, cu->user->nick, reason);
			}
			else if (strcasecmp(action, "GLINE") == 0)
			{
				kline_sts(me.name, "*", cu->user->host, 604800, reason);
			}

			notice(opersvs.nick, origin, "\2CLEARCHAN\2: \2%s\2 hit \2%s2\2!%s@%s {%s}", action, cu->user->nick, cu->user->user, cu->user->host, cu->user->gecos);
			matches++;
		}
	}

	notice(opersvs.nick, origin, "\2%d\2 matches, \2%d\2 ignores for \2%s\2 on \2%s\2", matches, ignores, action, targchan);
}
