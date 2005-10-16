/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv FORWARD function
 *
 * $Id: forward.c 2923 2005-10-16 05:07:06Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/forward", FALSE, _modinit, _moddeinit,
	"$Id: forward.c 2923 2005-10-16 05:07:06Z kog $",
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
	user_t *u = user_find(origin), *tu;
	myuser_t *mu = u->myuser, *tmu;
	mymemo_t *memo, *newmemo;
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
		
		return;
	}
	else
		memonum = atoi(arg);
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}
	
	/* Check to see if any memos */
	if (!mu->memos.count)
	{
		notice(memosvs.nick, origin, "You have no memos to forward.");
		return;
	}

	/* Check to see if target user exists */
	if (!(tmu = myuser_find(target)))
	{
		notice(memosvs.nick, origin, "%s is not registered.", target);
		return;
	}
	
	/* Make sure target isn't sender */
	if (mu == tmu)
	{
		notice(memosvs.nick, origin, "You cannot send yourself a memo.");
		return;
	}
	
	/* Make sure arg is an int */
	if (!memonum)
	{
		notice(memosvs.nick, origin, "Invalid message index.");
		return;
	}
	
	/* check if targetuser has nomemo set */
	if (tmu->flags & MU_NOMEMO)
	{
		notice(memosvs.nick, origin,
			"\2%s\2 does not wish to receive memos.", target);

		return;
	}

	/* Check to see if memo n exists */
	if (memonum > mu->memos.count)
	{
		notice(memosvs.nick, origin, "Invalid memo number.");
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (tmu->memos.count >= me.mdlimit)
	{
		notice(memosvs.nick, origin, "Target inbox is full.");
		return;
	}

	/* Make sure we're not on ignore */
	LIST_FOREACH(n, tmu->memo_ignores.head)
	{
		if (!strcasecmp((char *)n->data, origin))
		{
			/* Lie... change this if you want it to fail silent */
			notice(memosvs.nick, origin, "The memo has been successfully forwarded to %s.", target);
			return;
		}
	}
	
	/* Go to forwarding memos */
	LIST_FOREACH(n, mu->memos.head)
	{
		if (i == memonum)
		{
			/* should have some function for send here...  ask nenolod*/
			memo = (mymemo_t *)n->data;
			newmemo = smalloc(sizeof(mymemo_t));
			
			/* Create memo */
			newmemo->sent = CURRTIME;
			newmemo->status = MEMO_NEW;
			strlcpy(newmemo->sender,mu->name,NICKLEN);
			strlcpy(newmemo->text,memo->text,MEMOLEN);
			
			/* Create node, add to their linked list of memos */
			temp = node_create();
			node_add(newmemo, temp, &tmu->memos);
			tmu->memoct_new++;
		}
		
		i++;
	}
	
	/* Should we email this? */
	if (tmu->flags & MU_EMAILMEMOS)
	{
		sendemail(tmu->name, memo->text, 4);
		notice(memosvs.nick, origin, "Your memo has been emailed to %s.", target);
		return;
	}

	/* Note: do not disclose other nicks they're logged in with
	 * -- jilles */
	tu = user_find_named(target);
	if (tu != NULL && tu->myuser == tmu)
	{
		notice(memosvs.nick, origin, "%s is currently online, and you may talk directly, by sending a private message.", target);
	}
	if (!irccmp(origin, mu->name))
		myuser_notice(memosvs.nick, tmu, "You have a new forwarded memo from %s.", mu->name);
	else
		myuser_notice(memosvs.nick, tmu, "You have a new forwarded memo from %s (nick: %s).", mu->name, origin);

	notice(memosvs.nick, origin, "The memo has been successfully forwarded to %s.", target);
	return;
}
