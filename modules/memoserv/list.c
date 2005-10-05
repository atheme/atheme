/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv LIST function
 *
 * $Id: list.c 2597 2005-10-05 06:37:06Z kog $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 2597 2005-10-05 06:37:06Z kog $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_list(char *origin);

command_t ms_list = { "LIST", "Lists all of your memos",
                        AC_NONE, ms_cmd_list };

list_t *ms_cmdtree;

void _modinit(module_t *m)
{
	ms_cmdtree = module_locate_symbol("memoserv/main", "ms_cmdtree");
        command_add(&ms_list, ms_cmdtree);
}

void _moddeinit()
{
	command_delete(&ms_list, ms_cmdtree);
}

static void ms_cmd_list(char *origin)
{
	/* misc structs etc */
	user_t *u = user_find(origin);
	myuser_t *mu = u->myuser;
	mymemo_t *memo;
	node_t *n;
	int i = 0;
	
	//head, tail, count
	//LIST_FOREACH(n, cs_fcmdtree->head)
	//LIST_FOREACH(n, head)
	
	notice(memosvs.nick, origin, "You have %d memo%s.", mu->memos.count, 
		(!mu->memos.count || mu->memos.count > 1) ? "s" : "");
	
	/* check to see if any memos */
	if (!mu->memos.count)
		return;

	/* Go to listing memos */
	notice(memosvs.nick, origin, "");
	
	LIST_FOREACH(n, mu->memos.head)
	{
		i++;
		memo = (mymemo_t *)n->data;
		notice(memosvs.nick, origin, "%d - %s ",i,memo->sender);
	}
	
	/*
	char	 sender[NICKLEN];
	char 	 subject[30];
	char 	 text[MEMOLEN];
	time_t	 sent;
	uint32_t status;
	list_t	 metadata;
	*/
	
	return;
}
