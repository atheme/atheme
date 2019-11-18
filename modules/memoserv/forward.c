/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the Memoserv FORWARD function
 */

#include <atheme.h>

static void
ms_cmd_forward(struct sourceinfo *si, int parc, char *parv[])
{
	// Misc structs etc
	struct user *tu;
	struct myuser *tmu;
	struct mymemo *memo, *newmemo;
	mowgli_node_t *n, *temp;
	unsigned int i = 1, memonum = 0;
	struct service *const memoserv = service_find("memoserv");

	// Grab args
	char *target = parv[0];
	char *arg = parv[1];

	// Arg validator
	if (! target || ! arg || ! string_to_uint(arg, &memonum))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FORWARD");
		command_fail(si, fault_needmoreparams, _("Syntax: FORWARD <account> <memo number>"));
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	// Check to see if any memos
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("You have no memos to forward."));
		return;
	}

	// Check to see if target user exists
	if (!(tmu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	// Make sure target isn't sender
	if (si->smu == tmu)
	{
		command_fail(si, fault_noprivs, _("You cannot send yourself a memo."));
		return;
	}

	// Make sure arg is an int
	if (!memonum)
	{
		command_fail(si, fault_badparams, _("Invalid message index."));
		return;
	}

	// check if targetuser has nomemo set
	if (tmu->flags & MU_NOMEMO)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 does not wish to receive memos."), target);

		return;
	}

	// Check to see if memo n exists
	if (memonum > si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("Invalid memo number."));
		return;
	}

	// Check to make sure target inbox not full
	if (tmu->memos.count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Target inbox is full."));
		logcommand(si, CMDLOG_SET, "failed FORWARD to \2%s\2 (target inbox full)", entity(tmu)->name);
		return;
	}

	// rate limit it -- jilles
	if (CURRTIME - si->smu->memo_ratelimit_time > MEMO_MAX_TIME)
		si->smu->memo_ratelimit_num = 0;
	if (si->smu->memo_ratelimit_num > MEMO_MAX_NUM && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("Too many memos; please wait a while and try again"));
		return;
	}
	si->smu->memo_ratelimit_num++;
	si->smu->memo_ratelimit_time = CURRTIME;

	// Make sure we're not on ignore
	MOWGLI_ITER_FOREACH(n, tmu->memo_ignores.head)
	{
		struct mynick *mn;
		struct myuser *mu;

		if (nicksvs.no_nick_ownership)
			mu = myuser_find((const char *)n->data);
		else
		{
			mn = mynick_find((const char *)n->data);
			mu = mn != NULL ? mn->owner : NULL;
		}
		if (mu == si->smu)
		{
			// Lie... change this if you want it to fail silent
			logcommand(si, CMDLOG_SET, "failed FORWARD to \2%s\2 (on ignore list)", entity(tmu)->name);
			command_success_nodata(si, _("The memo has been successfully forwarded to \2%s\2."), target);
			return;
		}
	}
	logcommand(si, CMDLOG_SET, "FORWARD: to \2%s\2", entity(tmu)->name);

	// Go to forwarding memos
	MOWGLI_ITER_FOREACH(n, si->smu->memos.head)
	{
		if (i == memonum)
		{
			// should have some function for send here...  ask nenolod
			memo = (struct mymemo *)n->data;
			newmemo = smalloc(sizeof *newmemo);

			// Create memo
			newmemo->sent = CURRTIME;
			mowgli_strlcpy(newmemo->sender, entity(si->smu)->name, sizeof newmemo->sender);
			mowgli_strlcpy(newmemo->text, memo->text, sizeof newmemo->text);

			// Create node, add to their linked list of memos
			temp = mowgli_node_create();
			mowgli_node_add(newmemo, temp, &tmu->memos);
			tmu->memoct_new++;

			// Should we email this?
			if (tmu->flags & MU_EMAILMEMOS)
			{
				sendemail(si->su, tmu, EMAIL_MEMO, tmu->email, memo->text);
			}
		}
		i++;
	}

	// Note: do not disclose other nicks they're logged in with  -- jilles
	tu = user_find_named(target);
	if (tu != NULL && tu->myuser == tmu)
	{
		command_success_nodata(si, _("%s is currently online, and you may talk directly, by sending a private message."), target);
	}
	if (si->su == NULL || !irccasecmp(si->su->nick, entity(si->smu)->name))
		myuser_notice(si->service->nick, tmu, "You have a new forwarded memo from %s (%zu).", entity(si->smu)->name, MOWGLI_LIST_LENGTH(&tmu->memos));
	else
		myuser_notice(si->service->nick, tmu, "You have a new forwarded memo from %s (nick: %s) (%zu).", entity(si->smu)->name, si->su->nick, MOWGLI_LIST_LENGTH(&tmu->memos));

	myuser_notice(si->service->nick, tmu, "To read it, type \2/msg %s READ %zu\2",
	              memoserv->disp, MOWGLI_LIST_LENGTH(&tmu->memos));

	command_success_nodata(si, _("The memo has been successfully forwarded to \2%s\2."), target);
	return;
}

static struct command ms_forward = {
	.name           = "FORWARD",
	.desc           = N_("Forwards a memo."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &ms_cmd_forward,
	.help           = { .path = "memoserv/forward" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "memoserv/main")

        service_named_bind_command("memoserv", &ms_forward);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_forward);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/forward", MODULE_UNLOAD_CAPABILITY_OK)
