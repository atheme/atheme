/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv FORWARD function
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/forward", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_forward(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_forward = { "FORWARD", N_(N_("Forwards a memo.")),
                        AC_NONE, 2, ms_cmd_forward, { .path = "memoserv/forward" } };

void _modinit(module_t *m)
{
        service_named_bind_command("memoserv", &ms_forward);
}

void _moddeinit()
{
	service_named_unbind_command("memoserv", &ms_forward);
}

static void ms_cmd_forward(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	user_t *tu;
	myuser_t *tmu;
	mymemo_t *memo, *newmemo;
	mowgli_node_t *n, *temp;
	unsigned int i = 1, memonum = 0;
	
	/* Grab args */
	char *target = parv[0];
	char *arg = parv[1];
	
	/* Arg validator */
	if (!target || !arg)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "FORWARD");
		
		command_fail(si, fault_needmoreparams, 
			"Syntax: FORWARD <account> <memo number>");
		
		return;
	}
	else
		memonum = atoi(arg);
	
	/* user logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	
	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, _("You need to verify your email address before you may send memos."));
		return;
	}

	/* Check to see if any memos */
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("You have no memos to forward."));
		return;
	}

	/* Check to see if target user exists */
	if (!(tmu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}
	
	/* Make sure target isn't sender */
	if (si->smu == tmu)
	{
		command_fail(si, fault_noprivs, _("You cannot send yourself a memo."));
		return;
	}
	
	/* Make sure arg is an int */
	if (!memonum)
	{
		command_fail(si, fault_badparams, _("Invalid message index."));
		return;
	}
	
	/* check if targetuser has nomemo set */
	if (tmu->flags & MU_NOMEMO)
	{
		command_fail(si, fault_noprivs,
			"\2%s\2 does not wish to receive memos.", target);

		return;
	}

	/* Check to see if memo n exists */
	if (memonum > si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("Invalid memo number."));
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (tmu->memos.count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Target inbox is full."));
		logcommand(si, CMDLOG_SET, "failed FORWARD to \2%s\2 (target inbox full)", entity(tmu)->name);
		return;
	}

	/* rate limit it -- jilles */
	if (CURRTIME - si->smu->memo_ratelimit_time > MEMO_MAX_TIME)
		si->smu->memo_ratelimit_num = 0;
	if (si->smu->memo_ratelimit_num > MEMO_MAX_NUM && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("Too many memos; please wait a while and try again"));
		return;
	}
	si->smu->memo_ratelimit_num++;
	si->smu->memo_ratelimit_time = CURRTIME;

	/* Make sure we're not on ignore */
	MOWGLI_ITER_FOREACH(n, tmu->memo_ignores.head)
	{
		mynick_t *mn;
		myuser_t *mu;

		if (nicksvs.no_nick_ownership)
			mu = myuser_find((const char *)n->data);
		else
		{
			mn = mynick_find((const char *)n->data);
			mu = mn != NULL ? mn->owner : NULL;
		}
		if (mu == si->smu)
		{
			/* Lie... change this if you want it to fail silent */
			logcommand(si, CMDLOG_SET, "failed FORWARD to \2%s\2 (on ignore list)", entity(tmu)->name);
			command_success_nodata(si, _("The memo has been successfully forwarded to \2%s\2."), target);
			return;
		}
	}
	logcommand(si, CMDLOG_SET, "FORWARD: to \2%s\2", entity(tmu)->name);
	
	/* Go to forwarding memos */
	MOWGLI_ITER_FOREACH(n, si->smu->memos.head)
	{
		if (i == memonum)
		{
			/* should have some function for send here...  ask nenolod*/
			memo = (mymemo_t *)n->data;
			newmemo = smalloc(sizeof(mymemo_t));
			
			/* Create memo */
			newmemo->sent = CURRTIME;
			newmemo->status = 0;
			strlcpy(newmemo->sender,entity(si->smu)->name,NICKLEN);
			strlcpy(newmemo->text,memo->text,MEMOLEN);
			
			/* Create node, add to their linked list of memos */
			temp = mowgli_node_create();
			mowgli_node_add(newmemo, temp, &tmu->memos);
			tmu->memoct_new++;
		
			/* Should we email this? */
			if (tmu->flags & MU_EMAILMEMOS)
			{
				sendemail(si->su, EMAIL_MEMO, tmu, memo->text);
			}
		}
		i++;
	}

	/* Note: do not disclose other nicks they're logged in with
	 * -- jilles */
	tu = user_find_named(target);
	if (tu != NULL && tu->myuser == tmu)
	{
		command_success_nodata(si, _("%s is currently online, and you may talk directly, by sending a private message."), target);
	}
	if (si->su == NULL || !irccasecmp(si->su->nick, entity(si->smu)->name))
		myuser_notice(si->service->nick, tmu, "You have a new forwarded memo from %s (%zu).", entity(si->smu)->name, MOWGLI_LIST_LENGTH(&tmu->memos));
	else
		myuser_notice(si->service->nick, tmu, "You have a new forwarded memo from %s (nick: %s) (%zu).", entity(si->smu)->name, si->su->nick, MOWGLI_LIST_LENGTH(&tmu->memos));
	myuser_notice(si->service->nick, tmu, _("To read it, type /%s%s READ %zu"),
				ircd->uses_rcommand ? "" : "msg ", si->service->disp, MOWGLI_LIST_LENGTH(&tmu->memos));

	command_success_nodata(si, _("The memo has been successfully forwarded to \2%s\2."), target);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
