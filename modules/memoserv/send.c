/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SEND function
 *
 * $Id: send.c 2711 2005-10-06 09:53:48Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/send", FALSE, _modinit, _moddeinit,
	"$Id: send.c 2711 2005-10-06 09:53:48Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_send(char *origin);

command_t ms_send = { "SEND", "Sends a memo to a user",
                        AC_NONE, ms_cmd_send };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_send, ms_cmdtree);
	
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
	help_addentry(ms_helptree, "SEND", "help/memoserv/send", NULL);
}

void _moddeinit()
{
	command_delete(&ms_send, ms_cmdtree);
	help_delentry(ms_helptree, "SEND");
}

static void ms_cmd_send(char *origin)
{
	/* misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu;
	node_t *n;
	mymemo_t *memo;
	
	/* Grab args */
	char *target = strtok(NULL, " ");
	char *m = strtok(NULL,"");
	
	/* Arg validation */
	if (!target || !m)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2SEND\2.");
		
		notice(memosvs.nick, origin, 
			"Syntax: SEND <user> <subject> <memo>");
		
		return;
	}
	
	/* user logged in? */
	if (!u->myuser)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}
	
	/* See if target is valid */
	if (!(mu = myuser_find(target))) 
	{
		notice(memosvs.nick, origin, 
			"\2%s\2 is not a registered account", target);
		
		return;
	}
	
	/* Make sure target is not sender */
	if (!(strcasecmp(origin,target)))
	{
		notice(memosvs.nick, origin, "You cannot send yourself a memo.");
		return;
	}

	/* Does the user allow memos? --pfish */
	if (mu->flags & MU_NOMEMO)
	{
		notice(memosvs.nick,origin,
			"\2%s\2 does not wish to receive memos.", target);

		return;
	}
	
	/* Check for memo text length -- includes/common.h */
	if (strlen(m) > MEMOLEN)
	{
		notice(memosvs.nick, origin, 
			"Please make sure your memo is less than 128 characters");
		
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (mu->memos.count >= me.mdlimit)
	{
		notice(memosvs.nick, origin, "%s's inbox is full", target);
		free(memo);
		return;
	}
	
	/* Malloc and populate struct */
	memo = smalloc(sizeof(mymemo_t));
	memo->sent = CURRTIME;
	memo->status = MEMO_NEW;
	strlcpy(memo->sender,origin,NICKLEN);
	strlcpy(memo->text,m,MEMOLEN);
	
	/* Create a linked list node and add to memos */
	n = node_create();
	node_add(memo, n, &mu->memos);
	mu->memoct_new++;

	/* Should we email this? */
        if (mu->flags & MU_EMAILMEMOS)
        {
		sendemail(mu->name, memo->text, 4);
		notice(memosvs.nick, origin, "Your memo has been emailed to %s.", target);
                return;
        }

	
	/* Is the user online? If so, tell them about the new memo. */
	u = user_find_named(target);
	if (u->myuser)
	{
		notice(memosvs.nick, origin, "%s is currently online, and you may talk directly, by sending a private message.", target);
		notice(memosvs.nick, target, "You have a new memo from %s.", origin);
	}

	/* Tell user memo sent, return */
	notice(memosvs.nick, origin, "The memo has been successfully sent to %s.", target);
	return;
}	
