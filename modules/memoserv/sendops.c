/*
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SENDOPS function
 *
 * $Id: sendops.c 8327 2007-05-26 11:19:15Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/sendops", FALSE, _modinit, _moddeinit,
	"$Id: sendops.c 8327 2007-05-26 11:19:15Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_sendops(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_sendops = { "SENDOPS", N_("Sends a memo to all ops on a channel."),
                          AC_NONE, 2, ms_cmd_sendops };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_sendops, ms_cmdtree);
	help_addentry(ms_helptree, "SENDOPS", "help/memoserv/sendops", NULL);
}

void _moddeinit()
{
	command_delete(&ms_sendops, ms_cmdtree);
	help_delentry(ms_helptree, "SENDOPS");
}

static void ms_cmd_sendops(sourceinfo_t *si, int parc, char *parv[])
{
	/* misc structs etc */
	user_t *tu;
	myuser_t *tmu;
	node_t *n, *tn;
	mymemo_t *memo;
	mychan_t *mc;
	int sent = 0, tried = 0;
	boolean_t ignored;

	/* Grab args */
	char *target = parv[0];
	char *m = parv[1];
	
	/* Arg validation */
	if (!target || !m)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "SENDOPS");
		
		command_fail(si, fault_needmoreparams, 
			"Syntax: SENDOPS <channel> <memo>");
		
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
	si->smu->memo_ratelimit_num++;
	si->smu->memo_ratelimit_time = CURRTIME;

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
	
	mc = mychan_find(target);

	if (mc == NULL)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}

	if (!chanacs_user_has_flag(mc, si->su, CA_ACLVIEW))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	LIST_FOREACH(tn, mc->chanacs.head)
	{
		chanacs_t *ca = (chanacs_t *) tn->data;
		tmu = ca->myuser;

		if (!(ca->level & (CA_OP | CA_AUTOOP)) || tmu == NULL || tmu == si->smu)
			continue;

		tried++;

		/* Does the user allow memos? --pfish */
		if (tmu->flags & MU_NOMEMO)
			continue;

		/* Check to make sure target inbox not full */
		if (tmu->memos.count >= me.mdlimit)
			continue;

		/* As in SEND to a single user, make ignore fail silently */
		sent++;

		/* Make sure we're not on ignore */
		ignored = FALSE;
		LIST_FOREACH(n, tmu->memo_ignores.head)
		{
			if (!strcasecmp((char *)n->data, si->smu->name))
				ignored = TRUE;
		}
		if (ignored)
			continue;

		/* Malloc and populate struct */
		memo = smalloc(sizeof(mymemo_t));
		memo->sent = CURRTIME;
		memo->status = MEMO_NEW;
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
				continue;
		}

		/* Is the user online? If so, tell them about the new memo. */
		if (si->su == NULL || !irccasecmp(si->su->nick, si->smu->name))
			myuser_notice(memosvs.nick, tmu, "You have a new memo from %s.", si->smu->name);
		else
			myuser_notice(memosvs.nick, tmu, "You have a new memo from %s (nick: %s).", si->smu->name, si->su->nick);
	}

	/* Tell user memo sent, return */
	logcommand(si, CMDLOG_SET, "SENDOPS to %s (%d/%d sent)", mc->name, sent, tried);
	command_success_nodata(si, _("The memo has been successfully sent to %d ops on \2%s\2."), sent, mc->name);
	return;
}	

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
