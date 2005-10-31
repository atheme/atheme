/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv IGNORE functions
 *
 * $Id: ignore.c 3335 2005-10-31 03:48:14Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/ignore", FALSE, _modinit, _moddeinit,
	"$Id: ignore.c 3335 2005-10-31 03:48:14Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_ignore(char *origin);
static void ms_cmd_ignore_add(char *origin, char *target);
static void ms_cmd_ignore_del(char *origin, char *target);
static void ms_cmd_ignore_clear(char *origin, char *arg);
static void ms_cmd_ignore_list(char *origin, char *arg);

command_t ms_ignore = { "IGNORE", "Ignores a memo", AC_NONE, ms_cmd_ignore };
fcommand_t ms_ignore_add = { "ADD", AC_NONE, ms_cmd_ignore_add };
fcommand_t ms_ignore_del = { "DEL", AC_NONE, ms_cmd_ignore_del };
fcommand_t ms_ignore_ignore = { "CLEAR", AC_NONE, ms_cmd_ignore_clear };
fcommand_t ms_ignore_list = { "LIST", AC_NONE, ms_cmd_ignore_list };

list_t *ms_cmdtree;
list_t *ms_helptree;
list_t ms_ignore_cmds;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
       command_add(&ms_ignore, ms_cmdtree);
	
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
	help_addentry(ms_helptree, "IGNORE", "help/memoserv/ignore", NULL);
	
	/* Add sub-commands */
	fcommand_add(&ms_ignore_add, &ms_ignore_cmds);
	fcommand_add(&ms_ignore_del, &ms_ignore_cmds);
	fcommand_add(&ms_ignore_ignore, &ms_ignore_cmds);
	fcommand_add(&ms_ignore_list, &ms_ignore_cmds);
}

void _moddeinit()
{
	command_delete(&ms_ignore, ms_cmdtree);
	help_delentry(ms_helptree, "IGNORE");
	
	/* Delete sub-commands */
	fcommand_delete(&ms_ignore_add, &ms_ignore_cmds);
	fcommand_delete(&ms_ignore_del, &ms_ignore_cmds);
	fcommand_delete(&ms_ignore_ignore, &ms_ignore_cmds);
	fcommand_delete(&ms_ignore_list, &ms_ignore_cmds);
}

static void ms_cmd_ignore(char *origin)
{	
	/* Grab args */
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	char *cmd = strtok(NULL, " ");
	char *arg = strtok(NULL, " ");
	
	/* Bad/missing arg */
	if (!cmd)
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
	
	fcommand_exec(memosvs.nick, arg, origin, cmd, &ms_ignore_cmds);
}

static void ms_cmd_ignore_add(char *origin, char *target)
{
	/* Misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser, *tmu;
	node_t *n,*node;
	char *temp;
	
	/* Arg check*/
	if (target == NULL)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2IGNORE\2.");
		
		notice(memosvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>");
		return;
	}
	
	/* User attempting to ignore themself? */
	if (!strcasecmp(target,mu->name))
	{
		notice(memosvs.nick, origin, "Silly wabbit, you can't ignore yourself.");
		return;
	}
	
	/* Does the target account exist? */
	if (!(tmu = myuser_find(target)))
	{
		notice(memosvs.nick, origin, "%s is not registered.", target);
		return;
	}
	
	/* Ignore list is full */
	if (mu->memo_ignores.count >= MAXMSIGNORES)
	{
		notice(memosvs.nick, origin, "Your ignore list is full, please DEL an account.");
		return;
	}
		
	/* Iterate through list, make sure target not in it, if last node append */
	LIST_FOREACH(n, mu->memo_ignores.head)
	{
		temp = (char *)n->data;
		
		/* Already in the list */
		if (!strcasecmp(temp,target))
		{
			notice(memosvs.nick, origin, "Account %s is already in your ignore list.", temp);
			return;
		}
	}
	
	/* Add to ignore list */
	temp = sstrdup(target);
	
	node = node_create();
	node_add(temp, node, &mu->memo_ignores);
	notice(memosvs.nick, origin, "Account %s added to your ignore list.", target);
	return;
}

static void ms_cmd_ignore_del(char *origin, char *target)
{
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n, *tn;
	char *temp;
	
	/* Arg check*/
	if (target == NULL)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2IGNORE\2.");
		
		notice(memosvs.nick, origin, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>");
		return;
	}
	
	/* Iterate through list, make sure they're not in it, if last node append */
	LIST_FOREACH_SAFE(n, tn, mu->memo_ignores.head)
	{
		temp = (char *)n->data;
		
		/* Not using list or clear, but we've found our target in the ignore list */
		if (!strcasecmp(temp,target))
		{
			notice(memosvs.nick, origin, "Account %s removed from ignore list.", temp);
			node_del(n,&mu->memo_ignores);
			node_free(n);
			
			free(temp);
			
			return;
		}
	}
	
	notice(memosvs.nick, origin, "%s is not in your ignore list.", target);
	return;
}

static void ms_cmd_ignore_clear(char *origin, char *arg)
{
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n, *tn;
	
	/* Iterate through list, make sure they're not in it, if last node append */
	LIST_FOREACH_SAFE(n, tn, mu->memo_ignores.head)
	{
		free(n->data);
		node_del(n,&mu->memo_ignores);
		node_free(n);
	}
	
	/* Let them know list is clear */
	notice(memosvs.nick, origin, "Ignore list cleared.");
	return;
}

static void ms_cmd_ignore_list(char *origin, char *arg)
{
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n;
	uint8_t i = 1;
	
	/* Throw in list header */
	notice(memosvs.nick, origin, "");
	notice(memosvs.nick, origin, "Ignore list:");
	notice(memosvs.nick, origin, "-------------------------");
	notice(memosvs.nick, origin, "");
	
	/* Iterate through list, make sure they're not in it, if last node append */
	LIST_FOREACH(n, mu->memo_ignores.head)
	{
		notice(memosvs.nick, origin, "%d - %s ", i, (char *)n->data);
		i++;
	}
	
	/* Ignore list footer */
	if (i == 1)
		notice(memosvs.nick, origin, "list empty");

	notice(memosvs.nick, origin, "-------------------------");
	return;
}
