/*
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv READ function
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/read", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

#define MAX_READ_AT_ONCE 5

static void ms_cmd_read(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_read = { "READ", N_("Reads a memo."),
                        AC_NONE, 2, ms_cmd_read, { .path = "memoserv/read" } };

void _modinit(module_t *m)
{
        service_named_bind_command("memoserv", &ms_read);
}

void _moddeinit()
{
	service_named_unbind_command("memoserv", &ms_read);
}

static void ms_cmd_read(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	myuser_t *tmu;
	mymemo_t *memo, *receipt;
	mowgli_node_t *n;
	unsigned int i = 1, memonum = 0, numread = 0;
	char strfbuf[32];
	struct tm tm;
	bool readnew;
	
	/* Grab arg */
	char *arg1 = parv[0];
	
	if (!arg1)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "READ");
		
		command_fail(si, fault_needmoreparams, _("Syntax: READ <memo number>"));
		return;
	}
	
	/* user logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	
	/* Check to see if any memos */
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("You have no memos."));
		return;
	}
	
	memonum = atoi(arg1);
	readnew = !strcasecmp(arg1, "NEW");
	if (!readnew && !memonum)
	{
		command_fail(si, fault_badparams, _("Invalid message index."));
		return;
	}
	
	/* Check to see if memonum is greater than memocount */
	if (memonum > si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("Invalid message index."));
		return;
	}

	/* Go to reading memos */	
	MOWGLI_ITER_FOREACH(n, si->smu->memos.head)
	{
		memo = (mymemo_t *)n->data;
		if (i == memonum || (readnew && !(memo->status & MEMO_READ)))
		{
			tm = *localtime(&memo->sent);
			strftime(strfbuf, sizeof(strfbuf) - 1, 
				"%b %d %H:%M:%S %Y", &tm);
			
			if (!(memo->status & MEMO_READ))
			{
				memo->status |= MEMO_READ;
				si->smu->memoct_new--;
				tmu = myuser_find(memo->sender);
				
				/* If the sender is logged in, tell them the memo's been read */
				/* but not for channel memos */
				if (memo->status & MEMO_CHANNEL)
					;
				else if (strcasecmp(si->service->nick, memo->sender) && (tmu != NULL) && (tmu->logins.count > 0))
					myuser_notice(si->service->me->nick, tmu, "%s has read your memo, which was sent at %s", entity(si->smu)->name, strfbuf);
				else
				{
					/* If they have an account, their inbox is not full and they aren't memoserv */
					if ( (tmu != NULL) && (tmu->memos.count < me.mdlimit) && strcasecmp(si->service->nick, memo->sender))
					{
						/* Malloc and populate memo struct */
						receipt = smalloc(sizeof(mymemo_t));
						receipt->sent = CURRTIME;
						receipt->status = 0;
						strlcpy(receipt->sender, si->service->nick, NICKLEN);
						snprintf(receipt->text, MEMOLEN, "%s has read a memo from you sent at %s", entity(si->smu)->name, strfbuf);
						
						/* Attach to their linked list */
						n = mowgli_node_create();
						mowgli_node_add(receipt, n, &tmu->memos);
						tmu->memoct_new++;
					}
				}
			}
		
			command_success_nodata(si, 
				"\2Memo %d - Sent by %s, %s\2",i,memo->sender, strfbuf);
			
			command_success_nodata(si, 
				"------------------------------------------");
			
			command_success_nodata(si, "%s", memo->text);
			
			if (!readnew)
				return;
			if (++numread >= MAX_READ_AT_ONCE && si->smu->memoct_new > 0)
			{
				command_success_nodata(si, _("Stopping command after %d memos."), numread);
				return;
			}
		}
		i++;
	}

	if (readnew && numread == 0)
		command_fail(si, fault_nosuch_key, _("You have no new memos."));
	else if (readnew)
		command_success_nodata(si, _("Read %d memos."), numread);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
