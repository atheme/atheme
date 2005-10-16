/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv IGNORE function
 *
 * $Id: ignore.c 2913 2005-10-16 04:45:08Z kog $
 */

#include "atheme.h"

/* Defines local to this file - is this a bad practice? */
#define CASE_NONE 0
#define CASE_ADD 1
#define CASE_DEL 2
#define CASE_LIST 3
#define CASE_CLEAR 4

DECLARE_MODULE_V1
(
	"memoserv/ignore", FALSE, _modinit, _moddeinit,
	"$Id: ignore.c 2913 2005-10-16 04:45:08Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_ignore(char *origin);

command_t ms_ignore = { "IGNORE", "Ignores a memo",
                        AC_NONE, ms_cmd_ignore };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_ignore, ms_cmdtree);
	
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
	help_addentry(ms_helptree, "IGNORE", "help/memoserv/ignore", NULL);
}

void _moddeinit()
{
	command_delete(&ms_ignore, ms_cmdtree);
	help_delentry(ms_helptree, "IGNORE");
}

static void ms_cmd_ignore(char *origin)
{
	/* Misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser, *tmu;
	node_t *n, *link, *node;
	uint8_t cmd = CASE_NONE, i = 1;
	char *temp;
	
	/* Grab args */
	char *arg1 = strtok(NULL, " ");
	char *arg2 = strtok(NULL, " ");
	
	/* Bad/missing arg */
	if (!arg1)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2IGNORE\2.");
		
		notice(memosvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>");
		return;
	}
	
	/* Command check */
	if (!strcasecmp("ADD",arg1) && arg2 != NULL)
		cmd = CASE_ADD;
	else if (!strcasecmp("DEL",arg1) && arg2 != NULL)
		cmd = CASE_DEL;
	else if (!strcasecmp("LIST",arg1))
		cmd = CASE_LIST;
	else if (!strcasecmp("CLEAR",arg1))
		cmd = CASE_CLEAR;
	
	if (cmd == CASE_NONE)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2IGNORE\2.");
		
		notice(memosvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>");
		return;
	}
	
	/* User logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}
	
	/* User attempting to ignore themself? */
	if (cmd != CASE_LIST && cmd != CASE_CLEAR && !strcasecmp(arg2,origin))
	{
		notice(memosvs.nick, origin, "Silly wabbit, you can't ignore yourself.");
		return;
	}
	
	/* Does the target account exist? */
	if ((cmd != CASE_LIST && cmd != CASE_CLEAR)  && !(tmu = myuser_find(arg2)))
	{
		notice(memosvs.nick, origin, "%s is not a registered account.", arg2);
		return;
	}
	
	/* Ignore list is full */
	if (cmd == CASE_ADD && mu->memo_ignores.count >= MAXMSIGNORES)
	{
		notice(memosvs.nick, origin, "Your ignore list is full, please DEL an account.");
		return;
	}

	/* Throw in list header */
	if (cmd == CASE_LIST)
	{
		notice(memosvs.nick, origin, "");
		notice(memosvs.nick, origin, "Ignore list:");
		notice(memosvs.nick, origin, "-------------------------");
		notice(memosvs.nick, origin, "");
	}
		
	/* Iterate through list, make sure they're not in it, if last node append */
	LIST_FOREACH_SAFE(n, link, mu->memo_ignores.head)
	{
		temp = (char *)n->data;
		
		if (cmd == CASE_LIST)
			notice(memosvs.nick, origin, "%d - %s ", i, temp);
		
		if (cmd == CASE_CLEAR)
		{
			node_del(n,&mu->memo_ignores);
			node_free(n);
			free(temp);
		}
		
		/* Not using list or clear, but we've found our target in the ignore list */
		if (cmd != CASE_LIST  && cmd != CASE_CLEAR && !strcasecmp(temp,arg2))
		{
			/* Add only needs to see if account is unique */
			if (cmd == CASE_ADD)
			{
				notice(memosvs.nick, origin, "Account %s is already in your ignore list.", temp);
				return;
			}
			
			/* Delete the node */
			if (cmd == CASE_DEL)
			{
				notice(memosvs.nick, origin, "Account %s removed from ignore list.", temp);
				node_del(n,&mu->memo_ignores);
				node_free(n);
				
				free(temp);
				
				return;
			}
		}
		
		i++;
	}
	
	/* Add to ignore list */
	if (cmd == CASE_ADD && i < MAXMSIGNORES)
	{
		temp = malloc(sizeof(char[NICKLEN]));
		strlcpy(temp,arg2,NICKLEN);
		
		node = node_create();
		node_add(temp, node, &mu->memo_ignores);
		notice(memosvs.nick, origin, "Account %s added to your ignore list.", arg2);
		return;
	}
	
	/* Deletion notice if they weren't in your list */
	if (cmd == CASE_DEL)
	{
		notice(memosvs.nick, origin, "%s is not in your ignore list.", arg2);
		return;
	}
	
	/* Ignore list footer */
	if (cmd == CASE_LIST)
	{
		if (i == 1)
			notice(memosvs.nick, origin, "list empty");

		notice(memosvs.nick, origin, "-------------------------");
		return;
	}
	
	/* Let them know list is clear */
	if (cmd == CASE_CLEAR)
	{
		notice(memosvs.nick, origin, "Ignore list cleared.");
		return;
	}
}
