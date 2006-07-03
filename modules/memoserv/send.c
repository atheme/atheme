/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SEND function
 *
 * $Id: send.c 5686 2006-07-03 16:25:03Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/send", FALSE, _modinit, _moddeinit,
	"$Id: send.c 5686 2006-07-03 16:25:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_send(char *origin);

command_t ms_send = { "SEND", "Sends a memo to a user.",
                        AC_NONE, ms_cmd_send };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_send, ms_cmdtree);
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
	user_t *u = user_find_named(origin), *tu;
	myuser_t *tmu, *mu = u->myuser;
	node_t *n;
	mymemo_t *memo;
	
	/* Grab args */
	char *target = strtok(NULL, " ");
	char *m = strtok(NULL,"");
	
	/* Arg validation */
	if (!target || !m)
	{
		notice(memosvs.nick, origin, 
			STR_INSUFFICIENT_PARAMS, "SEND");
		
		notice(memosvs.nick, origin, 
			"Syntax: SEND <user> <memo>");
		
		return;
	}
	
	/* user logged in? */
	if (!u->myuser)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}

	if (u->myuser->flags & MU_WAITAUTH)
	{
		notice(memosvs.nick, origin, "You need to verify your email address before you may send memos.");
		return;
	}
	
	/* See if target is valid */
	if (!(tmu = myuser_find_ext(target))) 
	{
		notice(memosvs.nick, origin, 
			"\2%s\2 is not registered.", target);
		
		return;
	}
	
	/* Make sure target is not sender */
	if (tmu == u->myuser)
	{
		notice(memosvs.nick, origin, "You cannot send yourself a memo.");
		return;
	}

	/* Does the user allow memos? --pfish */
	if (tmu->flags & MU_NOMEMO)
	{
		notice(memosvs.nick,origin,
			"\2%s\2 does not wish to receive memos.", target);

		return;
	}
	
	/* Check for memo text length -- includes/common.h */
	if (strlen(m) >= MEMOLEN)
	{
		notice(memosvs.nick, origin, 
			"Please make sure your memo is less than %d characters", MEMOLEN);
		
		return;
	}

	if (*m == '\001')
	{
		notice(memosvs.nick, origin, "Your memo contains invalid characters.");
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (tmu->memos.count >= me.mdlimit)
	{
		notice(memosvs.nick, origin, "%s's inbox is full", target);
		logcommand(memosvs.me, u, CMDLOG_SET, "failed SEND to %s (target inbox full)", tmu->name);
		return;
	}

	/* rate limit it -- jilles */
	if (CURRTIME - mu->memo_ratelimit_time > MEMO_MAX_TIME)
		mu->memo_ratelimit_num = 0;
	if (mu->memo_ratelimit_num > MEMO_MAX_NUM)
	{
		notice(memosvs.nick, origin, "Too many memos; please wait a while and try again");
		return;
	}
	mu->memo_ratelimit_num++;
	mu->memo_ratelimit_time = CURRTIME;
	
	/* Make sure we're not on ignore */
	LIST_FOREACH(n, tmu->memo_ignores.head)
	{
		if (!strcasecmp((char *)n->data, mu->name))
		{
			/* Lie... change this if you want it to fail silent */
			logcommand(memosvs.me, u, CMDLOG_SET, "failed SEND to %s (on ignore list)", tmu->name);
			notice(memosvs.nick, origin, "The memo has been successfully forwarded to %s.", target);
			return;
		}
	}
	logcommand(memosvs.me, u, CMDLOG_SET, "SEND to %s", tmu->name);
	
	/* Malloc and populate struct */
	memo = smalloc(sizeof(mymemo_t));
	memo->sent = CURRTIME;
	memo->status = MEMO_NEW;
	strlcpy(memo->sender,u->myuser->name,NICKLEN);
	strlcpy(memo->text,m,MEMOLEN);
	
	/* Create a linked list node and add to memos */
	n = node_create();
	node_add(memo, n, &tmu->memos);
	tmu->memoct_new++;

	/* Should we email this? */
        if (tmu->flags & MU_EMAILMEMOS)
	{
		if (sendemail(u, EMAIL_MEMO, tmu, memo->text))
		{
			notice(memosvs.nick, origin, "Your memo has been emailed to %s.", target);
                	return;
		}
        }
	
	/* Is the user online? If so, tell them about the new memo. */
	/* Note: do not disclose other nicks they're logged in with
	 * -- jilles */
	tu = user_find_named(target);
	if (tu != NULL && tu->myuser == tmu)
	{
		notice(memosvs.nick, origin, "%s is currently online, and you may talk directly, by sending a private message.", target);
	}
	if (!irccmp(origin, u->myuser->name))
		myuser_notice(memosvs.nick, tmu, "You have a new memo from %s.", u->myuser->name);
	else
		myuser_notice(memosvs.nick, tmu, "You have a new memo from %s (nick: %s).", u->myuser->name, origin);

	/* Tell user memo sent, return */
	notice(memosvs.nick, origin, "The memo has been successfully sent to %s.", target);
	return;
}	
