/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SEND function
 *
 * $Id: send.c 2597 2005-10-05 06:37:06Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/send", FALSE, _modinit, _moddeinit,
	"$Id: send.c 2597 2005-10-05 06:37:06Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_send(char *origin);

command_t ms_send = { "SEND", "Sends a memo to a user",
                        AC_NONE, ms_cmd_send };

list_t *ms_cmdtree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_send, ms_cmdtree);
}

void _moddeinit()
{
	command_delete(&ms_send, ms_cmdtree);
}

static void ms_cmd_send(char *origin)
{
	/* misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu;
	node_t *n;
	list_t *memos;
	mymemo_t memo;
	
	/* Grab args */
	char *target = strtok(NULL, " ");
	char *subject = strtok(NULL, " ");
	char *m = strtok(NULL,"\0");
	
	/* Arg validation */
	if (!target || !subject || !m)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2SEND\2.");
		
		notice(memosvs.nick, origin, 
			"Syntax: SEND <user> <subject> <memo>");
		return;
	}
	
	/* See if target is valid */
	if (!(mu = myuser_find(target))) 
	{
		notice(memosvs.nick, origin, 
			"\2%s\2 is not a registered account", target);
		
		return;
	}
	
	/* Check for memo subject length */
	if (strlen(subject) > 30)
	{
		notice(memosvs.nick, origin, 
			"Please use a shorter subject on your memo", target);
		
		return;
	}
	
	/* Check for memo text length -- includes/common.h */
	if (strlen(m) > MEMOLEN)
	{
		notice(memosvs.nick, origin, 
			"Please make sure your memo is less than 128 characters");
		
		return;
	}
	
	/* Create a memo struct and populate */
	memo.sent = CURRTIME;
	memo.status = MEMO_NEW;
	strlcpy(memo.sender,origin,strlen(origin));
	strlcpy(memo.subject,subject,strlen(subject));
	strlcpy(memo.text,m,strlen(m));
	
	/* Create a linked list node and add to memos */
	memos = &mu->memos;
	n = node_create();
	node_add(&memo, n, memos);

	/* Tell user memo sent, return */
	notice(memosvs.nick, origin, "Memo sent.");
	return;
}
