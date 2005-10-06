/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv DELETE function
 *
 * $Id: delete.c 2695 2005-10-06 08:15:56Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/delete", FALSE, _modinit, _moddeinit,
	"$Id: delete.c 2695 2005-10-06 08:15:56Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_delete(char *origin);

command_t ms_delete = { "DELETE", "Deletes memos",
                        AC_NONE, ms_cmd_delete };
command_t ms_del = { "DEL", "Alias for DELETE",
			AC_NONE, ms_cmd_delete };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_delete, ms_cmdtree);
	command_add(&ms_del, ms_cmdtree);
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
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
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	node_t *n, *link;
	uint8_t i = 0, delcount = 0, memonum = 0;
	mymemo_t *memo;
	
	/* We only take 1 arg, and we ignore all others */
	char *arg1 = strtok(NULL, " ");
	
	/* Does the arg exist? */
	if (!arg1)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2DELETE\2.");
		
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
	
	/* Check to see if int or str - strncasecmp for buffer issues*/
        if (!strcasecmp("all",arg1))
	{
		delcount = mu->memos.count;
		
		/* boundary case - all called on 1 memo */
		if (mu->memos.count < 2)
			memonum = 1;
	}

	else
	{
		delcount = 1;
		memonum = atoi(arg1);
	}
	
	/* Make sure they didn't slip us an alphabetic index */
	if (!memonum && !delcount)
	{
		notice(memosvs.nick,origin,"Invalid message index.");
		return;
	}
	
	/* If int, does that index exist? And do we have something to delete? */
	if ((memonum && (memonum > mu->memos.count)) || (!memonum && !delcount))
	{
		notice(memosvs.nick,origin,"The specified memo doesn't exist.");
		return;
	}
	
	/* Iterate through memos, doing deletion */
	LIST_FOREACH_SAFE(n, link, mu->memos.head)
	{
		i++;
		
		if ((i == memonum) || delcount > 1)
		{			
			/* Free to node pool, remove from chain */
			node_del(n,&mu->memos);
			node_free(n);
			free(n->data);
		}
		
	}
	
	notice(memosvs.nick, origin, "%d memo%s deleted.",delcount, 
		(delcount == 1) ? "":"s");
	return;
}
