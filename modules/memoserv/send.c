/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv SEND function
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/send", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_send(sourceinfo_t *si, int parc, char *parv[]);
static unsigned int *maxmemos;


command_t ms_send = { "SEND", N_("Sends a memo to a user."),
                        AC_AUTHENTICATED, 2, ms_cmd_send, { .path = "memoserv/send" } };

void _modinit(module_t *m)
{
        service_named_bind_command("memoserv", &ms_send);
        MODULE_TRY_REQUEST_SYMBOL(m, maxmemos, "memoserv/main", "maxmemos");
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("memoserv", &ms_send);
}

static void ms_cmd_send(sourceinfo_t *si, int parc, char *parv[])
{
	/* misc structs etc */
	user_t *tu;
	myuser_t *tmu;
	mowgli_node_t *n;
	mymemo_t *memo;
	command_t *cmd;
	service_t *memoserv;
	
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
	
	memoserv = service_find("memoserv");
	if (memoserv == NULL)
		memoserv = si->service;

	if (*target != '#' && *target != '!')
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
		if (tmu->memos.count >= *maxmemos)
		{
			command_fail(si, fault_toomany, _("%s's inbox is full"), target);
			logcommand(si, CMDLOG_SET, "failed SEND to \2%s\2 (target inbox full)", entity(tmu)->name);
			return;
		}

	
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
				logcommand(si, CMDLOG_SET, "failed SEND to \2%s\2 (on ignore list)", entity(tmu)->name);
				command_success_nodata(si, _("The memo has been successfully sent to \2%s\2."), target);
				return;
			}
		}
		logcommand(si, CMDLOG_SET, "SEND: to \2%s\2", entity(tmu)->name);
	
		/* Malloc and populate struct */
		memo = smalloc(sizeof(mymemo_t));
		memo->sent = CURRTIME;
		memo->status = 0;
		mowgli_strlcpy(memo->sender,entity(si->smu)->name,NICKLEN);
		mowgli_strlcpy(memo->text,m,MEMOLEN);
	
		/* Create a linked list node and add to memos */
		n = mowgli_node_create();
		mowgli_node_add(memo, n, &tmu->memos);
		tmu->memoct_new++;

		/* Should we email this? */
	        if (tmu->flags & MU_EMAILMEMOS)
		{
			sendemail(si->su, tmu, EMAIL_MEMO, tmu->email, memo->text);
	        }
	
		/* Note: do not disclose other nicks they're logged in with
		 * -- jilles
		 *
		 * Actually, I don't see the point in this at all. If they want this information,
		 * they should use WHOIS. --nenolod
		 */
		tu = user_find_named(target);
		if (tu != NULL && tu->myuser == tmu)
			command_success_nodata(si, _("%s is currently online, and you may talk directly, by sending a private message."), target);

		/* Is the user online? If so, tell them about the new memo. */
		if (si->su == NULL || !irccasecmp(si->su->nick, entity(si->smu)->name))
			myuser_notice(memoserv->nick, tmu, "You have a new memo from %s (%zu).", entity(si->smu)->name, MOWGLI_LIST_LENGTH(&tmu->memos));
		else
			myuser_notice(memoserv->nick, tmu, "You have a new memo from %s (nick: %s) (%zu).", entity(si->smu)->name, si->su->nick, MOWGLI_LIST_LENGTH(&tmu->memos));
		myuser_notice(memoserv->nick, tmu, _("To read it, type /%s%s READ %zu"),
					ircd->uses_rcommand ? "" : "msg ", memoserv->disp, MOWGLI_LIST_LENGTH(&tmu->memos));

		/* Tell user memo sent */
		command_success_nodata(si, _("The memo has been successfully sent to \2%s\2."), target);
	}
	else if (*target == '#')
	{
		cmd = command_find(memoserv->commands, "SENDOPS");
		if (cmd != NULL)
			command_exec(memoserv, si, cmd, parc, parv);
		else
			command_fail(si, fault_nosuch_target, _("Channel memos are administratively disabled."));
	}
	else
	{
		cmd = command_find(memoserv->commands, "SENDGROUP");
		if (cmd != NULL)
			command_exec(memoserv, si, cmd, parc, parv);
		else
			command_fail(si, fault_nosuch_target, _("Group memos are administratively disabled."));
	}

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
