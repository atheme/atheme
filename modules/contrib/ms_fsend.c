/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv FSEND function
 *
 */

#include "atheme.h"

/* MEMOLEN + 8, so the "[FORCE] " string will fit */
#define FMEMOLEN 308

DECLARE_MODULE_V1
(
	"contrib/ms_fsend", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_fsend(sourceinfo_t *si, int parc, char *parv[]);

/* MARK is prolly the most appropriate priv (that I can think of), if you can
 * think of a better one, feel free to change it. --jdhore
 */
command_t ms_fsend = { "FSEND", N_("Forcibly sends a memo to a user."),
                        PRIV_MARK, 2, ms_cmd_fsend, { .path = "contrib/fsend" } };

void _modinit(module_t *m)
{
        service_named_bind_command("memoserv", &ms_fsend);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("memoserv", &ms_fsend);
}

static void ms_cmd_fsend(sourceinfo_t *si, int parc, char *parv[])
{
	/* misc structs etc */
	user_t *tu;
	myuser_t *tmu;
	mowgli_node_t *n;
	mymemo_t *memo;
	service_t *memoserv;
	
	/* Grab args */
	char *target = parv[0];
	char *m = parv[1];
	
	/* Arg validation */
	if (!target || !m)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "FSEND");
		
		command_fail(si, fault_needmoreparams, 
			"Syntax: FSEND <user> <memo>");
		
		return;
	}

	if (!si->smu)
        {
                command_fail(si, fault_noprivs, _("You are not logged in."));
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
	
		/* Check to make sure target inbox not full */
		if (tmu->memos.count >= me.mdlimit)
		{
			command_fail(si, fault_toomany, _("%s's inbox is full"), target);
			logcommand(si, CMDLOG_SET, "failed SEND to \2%s\2 (target inbox full)", entity(tmu)->name);
			return;
		}

		logcommand(si, CMDLOG_ADMIN, "FSEND: to \2%s\2", entity(tmu)->name);
	
		/* Malloc and populate struct */
		memo = smalloc(sizeof(mymemo_t));
		memo->sent = CURRTIME;
		memo->status = 0;
		mowgli_strlcpy(memo->sender,entity(si->smu)->name,NICKLEN);
		mowgli_strlcpy(memo->text, "[FORCE] ", FMEMOLEN);
		strncat(memo->text, m, FMEMOLEN);
	
		/* Create a linked list node and add to memos */
		n = mowgli_node_create();
		mowgli_node_add(memo, n, &tmu->memos);
		tmu->memoct_new++;

		/* Should we email this? */
	        if (tmu->flags & MU_EMAILMEMOS)
		{
			sendemail(si->su, EMAIL_MEMO, tmu, memo->text);
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
		command_fail(si, fault_nosuch_target, _("Channel memos may not be forced."));
	}
	else
	{
		command_fail(si, fault_nosuch_target, _("Group memos may not be forced."));
	}

	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
