/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv IGNORE functions
 *
 * $Id: ignore.c 6543 2006-09-29 15:09:51Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/ignore", FALSE, _modinit, _moddeinit,
	"$Id: ignore.c 6543 2006-09-29 15:09:51Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_ignore(sourceinfo_t *si, int parc, char *parv[]);
static void ms_cmd_ignore_add(char *origin, char *target);
static void ms_cmd_ignore_del(char *origin, char *target);
static void ms_cmd_ignore_clear(char *origin, char *arg);
static void ms_cmd_ignore_list(char *origin, char *arg);

command_t ms_ignore = { "IGNORE", "Ignores memos.", AC_NONE, 2, ms_cmd_ignore };
fcommand_t ms_ignore_add = { "ADD", AC_NONE, ms_cmd_ignore_add };
fcommand_t ms_ignore_del = { "DEL", AC_NONE, ms_cmd_ignore_del };
fcommand_t ms_ignore_clear = { "CLEAR", AC_NONE, ms_cmd_ignore_clear };
fcommand_t ms_ignore_list = { "LIST", AC_NONE, ms_cmd_ignore_list };

list_t *ms_cmdtree;
list_t *ms_helptree;
list_t ms_ignore_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

	command_add(&ms_ignore, ms_cmdtree);
	help_addentry(ms_helptree, "IGNORE", "help/memoserv/ignore", NULL);
	
	/* Add sub-commands */
	fcommand_add(&ms_ignore_add, &ms_ignore_cmds);
	fcommand_add(&ms_ignore_del, &ms_ignore_cmds);
	fcommand_add(&ms_ignore_clear, &ms_ignore_cmds);
	fcommand_add(&ms_ignore_list, &ms_ignore_cmds);
}

void _moddeinit()
{
	command_delete(&ms_ignore, ms_cmdtree);
	help_delentry(ms_helptree, "IGNORE");
	
	/* Delete sub-commands */
	fcommand_delete(&ms_ignore_add, &ms_ignore_cmds);
	fcommand_delete(&ms_ignore_del, &ms_ignore_cmds);
	fcommand_delete(&ms_ignore_clear, &ms_ignore_cmds);
	fcommand_delete(&ms_ignore_list, &ms_ignore_cmds);
}

static void ms_cmd_ignore(sourceinfo_t *si, int parc, char *parv[])
{	
	/* Grab args */
	char *cmd = parv[0];
	char *arg = parv[1];
	
	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "IGNORE");
		
		command_fail(si, fault_needmoreparams, "Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>");
		return;
	}
	
	/* User logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}
	
	fcommand_exec(memosvs.me, arg, si->su->nick, cmd, &ms_ignore_cmds);
}

static void ms_cmd_ignore_add(char *origin, char *target)
{
	/* Misc structs etc */
	user_t *u = user_find_named(origin);
	myuser_t *mu = u->myuser, *tmu;
	node_t *n,*node;
	char *temp;
	
	/* Arg check*/
	if (target == NULL)
	{
		notice(memosvs.nick, origin, 
			STR_INSUFFICIENT_PARAMS, "IGNORE");
		
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
	if (!(tmu = myuser_find_ext(target)))
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
	logcommand(memosvs.me, u, CMDLOG_SET, "IGNORE ADD %s", tmu->name);
	notice(memosvs.nick, origin, "Account %s added to your ignore list.", target);
	return;
}

static void ms_cmd_ignore_del(char *origin, char *target)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu = u->myuser;
	node_t *n, *tn;
	char *temp;
	
	/* Arg check*/
	if (target == NULL)
	{
		notice(memosvs.nick, origin, 
			STR_INSUFFICIENT_PARAMS, "IGNORE");
		
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
			logcommand(memosvs.me, u, CMDLOG_SET, "IGNORE DEL %s", temp);
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
	user_t *u = user_find_named(origin);
	myuser_t *mu = u->myuser;
	node_t *n, *tn;

	if (LIST_LENGTH(&mu->memo_ignores) == 0)
	{
		notice(memosvs.nick, origin, "Ignore list already empty.");
		return;
	}
	
	LIST_FOREACH_SAFE(n, tn, mu->memo_ignores.head)
	{
		free(n->data);
		node_del(n,&mu->memo_ignores);
		node_free(n);
	}
	
	/* Let them know list is clear */
	notice(memosvs.nick, origin, "Ignore list cleared.");
	logcommand(memosvs.me, u, CMDLOG_SET, "IGNORE CLEAR");
	return;
}

static void ms_cmd_ignore_list(char *origin, char *arg)
{
	user_t *u = user_find_named(origin);
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
