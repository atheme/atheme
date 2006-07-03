/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv DELETE function
 *
 * $Id: delete.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/delete", FALSE, _modinit, _moddeinit,
	"$Id: delete.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_delete(char *origin);

command_t ms_delete = { "DELETE", "Deletes memos.",
                        AC_NONE, ms_cmd_delete };
command_t ms_del = { "DEL", "Alias for DELETE",
			AC_NONE, ms_cmd_delete };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_delete, ms_cmdtree);
	command_add(&ms_del, ms_cmdtree);
	help_addentry(ms_helptree, "DELETE", "help/memoserv/delete", NULL);
}

void _moddeinit()
{
	command_delete(&ms_delete, ms_cmdtree);
	command_delete(&ms_del, ms_cmdtree);
	help_delentry(ms_helptree, "DELETE");
}

static void ms_cmd_delete(char *origin)
{
	/* Misc structs etc */
	user_t *u = user_find_named(origin);
	myuser_t *mu = u->myuser;
	node_t *n, *tn;
	uint8_t i = 0, delcount = 0, memonum = 0, deleteall = 0;
	mymemo_t *memo;
	
	/* We only take 1 arg, and we ignore all others */
	char *arg1 = strtok(NULL, " ");
	
	/* Does the arg exist? */
	if (!arg1)
	{
		notice(memosvs.nick, origin, 
			STR_INSUFFICIENT_PARAMS, "DELETE");
		
		notice(memosvs.nick, origin, "Syntax: DELETE ALL|message id");
		return;
	}
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}
	
	/* Do we have any memos? */
	if (!mu->memos.count)
	{
		notice(memosvs.nick,origin,"You have no memos to delete.");
		return;
	}
	
	/* Do we want to delete all memos? */
	if (!strcasecmp("all",arg1))
	{
		deleteall = 1;
	}
	
	else
	{
		memonum = atoi(arg1);
		
		/* Make sure they didn't slip us an alphabetic index */
		if (!memonum)
		{
			notice(memosvs.nick,origin,"Invalid message index.");
			return;
		}
		
		/* If int, does that index exist? And do we have something to delete? */
		if (memonum > mu->memos.count)
		{
			notice(memosvs.nick,origin,"The specified memo doesn't exist.");
			return;
		}
	}
	
	delcount = 0;
	
	/* Iterate through memos, doing deletion */
	LIST_FOREACH_SAFE(n, tn, mu->memos.head)
	{
		i++;
		
		if ((i == memonum) || (deleteall == 1))
		{
			delcount++;
			
			memo = (mymemo_t*) n->data;
			
			if (memo->status == MEMO_NEW)
				mu->memoct_new--;
			
			/* Free to node pool, remove from chain */
			node_del(n,&mu->memos);
			node_free(n);

			free(memo);
		}
		
	}
	
	notice(memosvs.nick, origin, "%d memo%s deleted.",delcount, 
		(delcount == 1) ? "":"s");
	
	return;
}
