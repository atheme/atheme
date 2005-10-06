/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv READ function
 *
 * $Id: list.c 2597 2005-10-05 06:37:06Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/read", FALSE, _modinit, _moddeinit,
	"$Id: list.c 2597 2005-10-05 06:37:06Z kog $",
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
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	mymemo_t *memo;
	node_t *n;
	int i = 1, memonum = 0;
	char strfbuf[32];
	struct tm tm;
	
	/* Grab arg */
	char *arg1 = strtok(NULL, " ");
	
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
	
	/* Check to see if any memos */
	if (!mu->memos.count)
	{
		notice(memosvs.nick, origin, "You have no memos.");
		return;
	}
	
	/* Check to see if memonum is greater than memocount */
	if (memonum > mu->memos.count)
	{
		notice(memosvs.nick, origin, "Invalid message index");
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
		
			notice(memosvs.nick, origin, 
				"\2Memo %d - Sent by %s, %s\2", i, memo->sender, strfbuf);
			
			notice(memosvs.nick, origin, 
				"------------------------------------------");
			
			notice(memosvs.nick, origin, "%s", memo->text);
			return;
		}
		
		i++;
	}
}
