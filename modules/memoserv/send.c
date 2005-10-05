/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SEND function
 *
 * $Id: send.c 2625 2005-10-05 23:01:11Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/send", FALSE, _modinit, _moddeinit,
	"$Id: send.c 2625 2005-10-05 23:01:11Z kog $",
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
	mymemo_t *memo = smalloc(sizeof(mymemo_t));
	
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
	
	/* See if target is valid */
	if (!(mu = myuser_find(target))) 
	{
		notice(memosvs.nick, origin, 
			"\2%s\2 is not a registered account", target);
		
		return;
	}
	
	/* Check for memo text length -- includes/common.h */
	if (strlen(m) > MEMOLEN)
	{
		notice(memosvs.nick, origin, 
			"Please make sure your memo is less than 128 characters");
		
		return;
	}
	
	/* Check to make sure target inbox not full  - nenolod suggested
	   config_options.metadata_limit, perhaps a conf var or conf.h? FIXME*/
	if (mu->memos.count > 30)
	{
		notice(memosvs.nick, origin, "%s's inbox is full", target);
		return;
	}
	
	/* Populate struct */
	memo->sent = CURRTIME;
	memo->status = MEMO_NEW;
	strlcpy(memo->sender,origin,NICKLEN);
	strlcpy(memo->text,m,MEMOLEN);
	
	/* Create a linked list node and add to memos */
	n = node_create();
	node_add(memo, n, &mu->memos);

	/* Tell user memo sent, return */
	notice(memosvs.nick, origin, "Memo sent.");
	return;
}
