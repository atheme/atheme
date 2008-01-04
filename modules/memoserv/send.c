/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SEND function
 *
 * $Id: send.c 8329 2007-05-27 13:31:59Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/send", FALSE, _modinit, _moddeinit,
	"$Id: send.c 8329 2007-05-27 13:31:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_send(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_send = { "SEND", N_("Sends a memo to a user."),
                        AC_NONE, 2, ms_cmd_send };

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

static void ms_cmd_send(sourceinfo_t *si, int parc, char *parv[])
{
	/* misc structs etc */
	user_t *tu;
	myuser_t *tmu;
	node_t *n;
	mymemo_t *memo;
	command_t *cmd;
	
	/* Grab args */
	char *target = parv[0];
	char *m = parv[1];
	
	/* Arg validation */
	if (!target || !m)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "SEND");
		
		command_fail(si, fault_needmoreparams, 
			"Syntax: SEND <user> <memo>");
		
		return;
	}
	
	/* user logged in? */
	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, _("You need to verify your email address before you may send memos."));
		return;
	}

	/* rate limit it -- jilles */
	if (CURRTIME - si->smu->memo_ratelimit_time > MEMO_MAX_TIME)
		si->smu->memo_ratelimit_num = 0;
	if (si->smu->memo_ratelimit_num > MEMO_MAX_NUM && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("You have used this command too many times; please wait a while and try again."));
		return;
	}

	/* Check for memo text length -- includes/common.h */
	if (strlen(m) >= MEMOLEN)
	{
		command_fail(si, fault_badparams, 
			"Please make sure your memo is less than %d characters", MEMOLEN);
		
		return;
	}

	/* Check to make sure the memo doesn't contain hostile CTCP responses.
	 * realistically, we'll probably want to check the _entire_ message for this... --nenolod
	 */
	if (*m == '\001')
	{
		command_fail(si, fault_badparams, _("Your memo contains invalid characters."));
		return;
	}
	
	if (*target != '#')
	{
		/* See if target is valid */
		if (!(tmu = myuser_find_ext(target))) 
		{
			command_fail(si, fault_nosuch_target, 
				"\2%s\2 is not registered.", target);
		
			return;
		}

		si->smu->memo_ratelimit_num++;
		si->smu->memo_ratelimit_time = CURRTIME;
	
		/* Does the user allow memos? --pfish */
		if (tmu->flags & MU_NOMEMO)
		{
			command_fail(si, fault_noprivs,
				"\2%s\2 does not wish to receive memos.", target);

			return;
		}
	
		/* Check to make sure target inbox not full */
		if (tmu->memos.count >= me.mdlimit)
		{
			command_fail(si, fault_toomany, _("%s's inbox is full"), target);
			logcommand(si, CMDLOG_SET, "failed SEND to %s (target inbox full)", tmu->name);
			return;
		}

	
		/* Make sure we're not on ignore */
		LIST_FOREACH(n, tmu->memo_ignores.head)
		{
			if (!strcasecmp((char *)n->data, si->smu->name))
			{
				logcommand(si, CMDLOG_SET, "failed SEND to %s (on ignore list)", tmu->name);
				command_success_nodata(si, _("The memo has been successfully sent to \2%s\2."), target);
				return;
			}
		}
		logcommand(si, CMDLOG_SET, "SEND to %s", tmu->name);
	
		/* Malloc and populate struct */
		memo = smalloc(sizeof(mymemo_t));
		memo->sent = CURRTIME;
		memo->status = 0;
		strlcpy(memo->sender,si->smu->name,NICKLEN);
		strlcpy(memo->text,m,MEMOLEN);
	
		/* Create a linked list node and add to memos */
		n = node_create();
		node_add(memo, n, &tmu->memos);
		tmu->memoct_new++;

		/* Should we email this? */
	        if (tmu->flags & MU_EMAILMEMOS)
		{
			if (sendemail(si->su, EMAIL_MEMO, tmu, memo->text))
			{
				command_success_nodata(si, _("Your memo has been emailed to \2%s\2."), target);
        	        	return;
			}
	        }
	
		/* Is the user online? If so, tell them about the new memo. */
		/* Note: do not disclose other nicks they're logged in with
		 * -- jilles
		 *
		 * Actually, I don't see the point in this at all. If they want this information,
		 * they should use WHOIS. --nenolod
		 */
		tu = user_find_named(target);
		if (tu != NULL && tu->myuser == tmu)
			command_success_nodata(si, _("%s is currently online, and you may talk directly, by sending a private message."), target);

		if (si->su == NULL || !irccasecmp(si->su->nick, si->smu->name))
			myuser_notice(memosvs.nick, tmu, "You have a new memo from %s.", si->smu->name);
		else
			myuser_notice(memosvs.nick, tmu, "You have a new memo from %s (nick: %s).", si->smu->name, si->su->nick);

		/* Tell user memo sent */
		command_success_nodata(si, _("The memo has been successfully sent to \2%s\2."), target);
	}
	else
	{
		cmd = command_find(si->service->cmdtree, "SENDOPS");
		if (cmd != NULL)
			command_exec(si->service, si, cmd, parc, parv);
		else
			command_fail(si, fault_nosuch_target, _("Channel memos are administratively disabled."));
	}

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
