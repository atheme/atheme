/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv LIST function
 *
 * $Id: list.c 2647 2005-10-06 01:20:29Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 2647 2005-10-06 01:20:29Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_list(char *origin);

command_t ms_list = { "LIST", "Lists all of your memos",
                        AC_NONE, ms_cmd_list };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_list, ms_cmdtree);
	
	ms_helptree = module_locate_symbol("memoserv/main", "ms_helptree");
	help_addentry(ms_helptree, "LIST", "help/memoserv/list", NULL);
}

void _moddeinit()
{
	command_delete(&ms_list, ms_cmdtree);
	help_delentry(ms_helptree, "LIST");
}

static void ms_cmd_list(char *origin)
{
	/* Misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	mymemo_t *memo;
	node_t *n;
	uint8_t i = 0;
	char strfbuf[32];
	struct tm tm;
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, origin, "You are not logged in.");
		return;
	}
		
	
	notice(memosvs.nick, origin, "You have %d memo%s.", mu->memos.count, 
		(!mu->memos.count || mu->memos.count > 1) ? "s":"");
	
	/* Check to see if any memos */
	if (!mu->memos.count)
		return;

	/* Go to listing memos */
	notice(memosvs.nick, origin, "");
	
	LIST_FOREACH(n, mu->memos.head)
	{
		i++;
		memo = (mymemo_t *)n->data;
		tm = *localtime(&memo->sent);
		
		strftime(strfbuf, sizeof(strfbuf) - 1, 
			"%b %d %H:%M:%S %Y", &tm);
		
		notice(memosvs.nick, origin, "%d - From %s sent %s",i,memo->sender,strfbuf);
	}
	
	return;
}
