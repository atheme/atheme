/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv READ function
 *
 * $Id: read.c 2767 2005-10-08 20:08:06Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/read", FALSE, _modinit, _moddeinit,
	"$Id: read.c 2767 2005-10-08 20:08:06Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_read(char *origin);

command_t ms_read = { "READ", "Reads a memo",
                        AC_NONE, ms_cmd_read };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_read, ms_cmdtree);
	
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
	help_addentry(ms_helptree, "READ", "help/memoserv/read", NULL);
}

void _moddeinit()
{
	command_delete(&ms_read, ms_cmdtree);
	help_delentry(ms_helptree, "READ");
}

static void ms_cmd_read(char *origin)
{
	/* Misc structs etc */
	user_t *u = user_find(origin), *tu;
	myuser_t *mu = u->myuser, *tmu;
	mymemo_t *memo, *receipt;
	node_t *n;
	int i = 1, memonum = 0;
	char strfbuf[32];
	struct tm tm;
	
	/* Grab arg */
	char *arg1 = strtok(NULL, " "), strbuf;
	
	/* Bad/missing arg -- how do I make sure it's a digit they fed me? */
	if (!arg1)
	{
		notice(memosvs.nick, origin, 
			"Insufficient parameters specified for \2READ\2.");
		
		notice(memosvs.nick, origin, "Syntax: READ <memo number>");
		return;
	}
	else
		memonum = atoi(arg1);
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}
	
	/* Check to see if any memos */
	if (!mu->memos.count)
	{
		notice(memosvs.nick, origin, "You have no memos.");
		return;
	}
	
	/* Is arg1 an int? */
	if (!memonum)
	{
		notice(memosvs.nick, origin, "Invalid message index.");
		return;
	}
	
	/* Check to see if memonum is greater than memocount */
	if (memonum > mu->memos.count)
	{
		notice(memosvs.nick, origin, "Invalid message index.");
		return;
	}

	/* Go to reading memos */	
	LIST_FOREACH(n, mu->memos.head)
	{
		if (i == memonum)
		{
			memo = (mymemo_t*) n->data;
			tm = *localtime(&memo->sent);
			strftime(strfbuf, sizeof(strfbuf) - 1, 
				"%b %d %H:%M:%S %Y", &tm);
			
			if (memo->status == MEMO_NEW)
			{
				memo->status = MEMO_READ;
				mu->memoct_new--;
				tu = user_find(memo->sender);
				tmu = tu->myuser;
				
				/* If the sender is logged in, tell them the memo's been read */
				if ( strcasecmp(memosvs.nick,memo->sender) && (tmu != NULL) )
					myuser_notice(memosvs.nick, tmu, "%s has read your memo, which was sent at %s", origin, strfbuf);			
				else
				{	
					tmu = myuser_find(memo->sender);
					
					/* If they have an account, their inbox is not full and they aren't memoserv */
					if ( (tmu != NULL) && (tmu->memos.count < me.mdlimit) && strcasecmp(memosvs.nick,memo->sender))
					{
						/* Malloc and populate memo struct */
						receipt = smalloc(sizeof(mymemo_t));
						memo->sent = CURRTIME;
						memo->status = MEMO_NEW;
						strlcpy(memo->sender,memosvs.nick,NICKLEN);
						snprintf(&strbuf, BUFSIZE, "%s has read a memo from you sent at %s", origin, strfbuf);
						strlcpy(memo->text,&strbuf,MEMOLEN);
						
						/* Attach to their linked list */
						n = node_create();
						node_add(memo, n, &tmu->memos);
						tmu->memoct_new++;
					}
				}
			}
		
			notice(memosvs.nick, origin, 
				"\2Memo %d - Sent by %s, %s\2",i,memo->sender, strfbuf);
			
			notice(memosvs.nick, origin, 
				"------------------------------------------");
			
			notice(memosvs.nick, origin, "%s", memo->text);
			
			return;
		}
		
		i++;
	}
}
