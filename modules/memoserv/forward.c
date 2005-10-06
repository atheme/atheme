/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv FORWARD function
 *
 * $Id: forward.c 2597 2005-10-05 06:37:06Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/forward", FALSE, _modinit, _moddeinit,
	"$Id: forward.c 2597 2005-10-05 06:37:06Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_forward(char *origin);

command_t ms_forward = { "FORWARD", "Forwards a memo",
                        AC_NONE, ms_cmd_forward };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_forward, ms_cmdtree);
	
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
	help_addentry(ms_helptree, "forward", "help/memoserv/forward", NULL);
}

void _moddeinit()
{
	command_delete(&ms_forward, ms_cmdtree);
	help_delentry(ms_helptree, "FORWARD");
}

static void ms_cmd_forward(char *origin)
{
	/* Misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser, *tmu;
	mymemo_t *memo, *newmemo = smalloc(sizeof(mymemo_t));
	node_t *n, *temp;
	uint8_t i = 1, memonum = 0;
	
	/* Grab args */
	char *target = strtok(NULL, " ");
	char *arg = strtok(NULL, " ");
	
	/* Arg validator */
	if (!target || !arg)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2FORWARD\2.");
		
		notice(memosvs.nick, origin, 
			"Syntax: FORWARD <account> <memo number>");
		
		free(newmemo);
		return;
	}
	else
		memonum = atoi(arg);
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		free(newmemo);
		return;
	}
	
	/* Check to see if any memos */
	if (!mu->memos.count)
	{
		notice(memosvs.nick, origin, "You have no memos to forward.");
		free(newmemo);
		return;
	}

	/* Check to see if target user exists */
	if (!(tmu = myuser_find(target)))
	{
		notice(memosvs.nick, origin, "%s is not registered.", target);
		free(newmemo);
		return;
	}
	
	/* Make sure target isn't sender */
	if (!(strcasecmp(origin,target)))
	{
		notice(memosvs.nick, origin, "You cannot send yourself a memo.");
		free(memo);
		return;
	}
	
	/* check if targetuser has nomemo set */
	if (tmu->flags & MU_NOMEMO)
	{
		notice(memosvs.nick, origin,
			"\2%s\2 does not wish to receive memos.", target);

		free(newmemo);
		return;
	}

	/* Check to see if memo n exists */
	if (memonum > mu->memos.count)
	{
		notice(memosvs.nick, origin, "Invalid memo number.");
		free(newmemo);
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (tmu->memos.count >= me.mdlimit)
	{
		notice(memosvs.nick, origin, "Target inbox is full.");
		free(newmemo);
		return;
	}

	
	/* Go to forwarding memos */
	LIST_FOREACH(n, mu->memos.head)
	{
		if (i == memonum)
		{
			/* should have some function for send here...  ask nenolod*/
			memo = (mymemo_t *)n->data;
			
			/* Create memo */
			newmemo->sent = CURRTIME;
			newmemo->status = MEMO_NEW;
			strlcpy(newmemo->sender,origin,NICKLEN);
			strlcpy(newmemo->text,memo->text,MEMOLEN);
			
			/* Create node, add to their linked list of memos */
			temp = node_create();
			node_add(newmemo, temp, &tmu->memos);
		}
		
		i++;
	}
	
	u = user_find_named(target);
	if (u->myuser)
	{
		notice(memosvs.nick, origin, "%s is currently online, and you may talk directly, by sending a private message.", target);
		notice(memosvs.nick, target, "You have a new forwarded memo from %s.", origin);
	}

	notice(memosvs.nick, origin, "The memo has been successfully forwarded to %s.", target);
	return;
}
