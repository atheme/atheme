/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv LIST function
 *
 * $Id: list.c 6543 2006-09-29 15:09:51Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/list", FALSE, _modinit, _moddeinit,
	"$Id: list.c 6543 2006-09-29 15:09:51Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_list(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_list = { "LIST", "Lists all of your memos.",
                        AC_NONE, 0, ms_cmd_list };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_list, ms_cmdtree);
	help_addentry(ms_helptree, "LIST", "help/memoserv/list", NULL);
}

void _moddeinit()
{
	command_delete(&ms_list, ms_cmdtree);
	help_delentry(ms_helptree, "LIST");
}

static void ms_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	mymemo_t *memo;
	node_t *n;
	uint8_t i = 0;
	char strfbuf[32];
	struct tm tm;
	
	/* user logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}
		
	
	command_success_nodata(si, "You have %d memo%s (%d new).",
		si->smu->memos.count, 
		(!si->smu->memos.count || si->smu->memos.count > 1) ? "s":"",
		si->smu->memoct_new);
	
	/* Check to see if any memos */
	if (!si->smu->memos.count)
		return;

	/* Go to listing memos */
	command_success_nodata(si, " ");
	
	LIST_FOREACH(n, si->smu->memos.head)
	{
		i++;
		memo = (mymemo_t *)n->data;
		tm = *localtime(&memo->sent);
		
		strftime(strfbuf, sizeof(strfbuf) - 1, 
			"%b %d %H:%M:%S %Y", &tm);
		
		if (memo->status == MEMO_NEW)
			command_success_nodata(si, "- %d From: %s Sent: %s [unread]",i,memo->sender,strfbuf);
		else
			command_success_nodata(si, "- %d From: %s Sent: %s",i,memo->sender,strfbuf);
	}
	
	return;
}
