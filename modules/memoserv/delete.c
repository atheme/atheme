/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv DELETE function
 *
 * $Id: delete.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/delete", FALSE, _modinit, _moddeinit,
	"$Id: delete.c 7779 2007-03-03 13:55:42Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_delete(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_delete = { "DELETE", "Deletes memos.",
                        AC_NONE, 1, ms_cmd_delete };
command_t ms_del = { "DEL", "Alias for DELETE",
			AC_NONE, 1, ms_cmd_delete };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_delete, ms_cmdtree);
	command_add(&ms_del, ms_cmdtree);
	help_addentry(ms_helptree, "DELETE", "help/memoserv/delete", NULL);
}

void _moddeinit()
{
	command_delete(&ms_delete, ms_cmdtree);
	command_delete(&ms_del, ms_cmdtree);
	help_delentry(ms_helptree, "DELETE");
}

static void ms_cmd_delete(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	node_t *n, *tn;
	uint8_t i = 0, delcount = 0, memonum = 0, deleteall = 0;
	mymemo_t *memo;
	
	/* We only take 1 arg, and we ignore all others */
	char *arg1 = parv[0];
	
	/* Does the arg exist? */
	if (!arg1)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "DELETE");
		
		command_fail(si, fault_needmoreparams, "Syntax: DELETE ALL|message id");
		return;
	}
	
	/* user logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}
	
	/* Do we have any memos? */
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nochange, "You have no memos to delete.");
		return;
	}
	
	/* Do we want to delete all memos? */
	if (!strcasecmp("all",arg1))
	{
		deleteall = 1;
	}
	
	else
	{
		memonum = atoi(arg1);
		
		/* Make sure they didn't slip us an alphabetic index */
		if (!memonum)
		{
			command_fail(si, fault_badparams, "Invalid message index.");
			return;
		}
		
		/* If int, does that index exist? And do we have something to delete? */
		if (memonum > si->smu->memos.count)
		{
			command_fail(si, fault_nosuch_key, "The specified memo doesn't exist.");
			return;
		}
	}
	
	delcount = 0;
	
	/* Iterate through memos, doing deletion */
	LIST_FOREACH_SAFE(n, tn, si->smu->memos.head)
	{
		i++;
		
		if ((i == memonum) || (deleteall == 1))
		{
			delcount++;
			
			memo = (mymemo_t*) n->data;
			
			if (memo->status == MEMO_NEW)
				si->smu->memoct_new--;
			
			/* Free to node pool, remove from chain */
			node_del(n, &si->smu->memos);
			node_free(n);

			free(memo);
		}
		
	}
	
	command_success_nodata(si, "%d memo%s deleted.", delcount, 
		(delcount == 1) ? "" : "s");
	
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
