/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2011 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the MemoServ SENDALL function
 */

#include <atheme.h>

static unsigned int *maxmemos;

static void
ms_cmd_sendall(struct sourceinfo *si, int parc, char *parv[])
{
	// misc structs etc
	struct myentity *mt;
	mowgli_node_t *n;
	struct mymemo *memo;
	unsigned int sent = 0, tried = 0;
	bool ignored;
	struct service *memoserv;
	struct myentity_iteration_state state;

	// Grab args
	char *m = parv[0];

	// Arg validation
	if (!m)
	{
		command_fail(si, fault_needmoreparams,
			STR_INSUFFICIENT_PARAMS, "SENDALL");

		command_fail(si, fault_needmoreparams,
			_("Syntax: SENDALL <memo>"));

		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	// rate limit it -- jilles
	if (CURRTIME - si->smu->memo_ratelimit_time > MEMO_MAX_TIME)
		si->smu->memo_ratelimit_num = 0;
	if (si->smu->memo_ratelimit_num > MEMO_MAX_NUM && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("You have used this command too many times; please wait a while and try again."));
		return;
	}

	// Check for memo text length -- includes/common.h
	if (strlen(m) > MEMOLEN)
	{
		command_fail(si, fault_badparams, _("Please make sure your memo is not greater than %u characters"), MEMOLEN);
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

	si->smu->memo_ratelimit_num++;
	si->smu->memo_ratelimit_time = CURRTIME;

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		struct myuser *tmu = user(mt);

		if (tmu == si->smu)
			continue;

		tried++;

		// Does the user allow memos? --pfish
		if (tmu->flags & MU_NOMEMO)
			continue;

		// Check to make sure target inbox not full
		if (tmu->memos.count >= *maxmemos)
			continue;

		// As in SEND to a single user, make ignore fail silently
		sent++;

		// Make sure we're not on ignore
		ignored = false;
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
				ignored = true;
		}
		if (ignored)
			continue;

		// Malloc and populate struct
		memo = smalloc(sizeof *memo);
		memo->sent = CURRTIME;
		memo->status = MEMO_CHANNEL;
		mowgli_strlcpy(memo->sender, entity(si->smu)->name, sizeof memo->sender);
		mowgli_strlcpy(memo->text, m, sizeof memo->text);

		// Create a linked list node and add to memos
		n = mowgli_node_create();
		mowgli_node_add(memo, n, &tmu->memos);
		tmu->memoct_new++;

		// Should we email this?
		if (tmu->flags & MU_EMAILMEMOS)
		{
			sendemail(si->su, tmu, EMAIL_MEMO, tmu->email, memo->text);
		}

		memoserv = service_find("memoserv");
		if (memoserv == NULL)
			memoserv = si->service;

		// Is the user online? If so, tell them about the new memo.
		if (si->su == NULL || !irccasecmp(si->su->nick, entity(si->smu)->name))
			myuser_notice(memoserv->nick, tmu, "You have a new memo from %s (%zu).", entity(si->smu)->name, MOWGLI_LIST_LENGTH(&tmu->memos));
		else
			myuser_notice(memoserv->nick, tmu, "You have a new memo from %s (nick: %s) (%zu).", entity(si->smu)->name, si->su->nick, MOWGLI_LIST_LENGTH(&tmu->memos));

		myuser_notice(si->service->nick, tmu, "To read it, type \2/msg %s READ %zu\2",
		              memoserv->disp, MOWGLI_LIST_LENGTH(&tmu->memos));
	}

	// Tell user memo sent, return
	if (sent > 4)
		command_add_flood(si, FLOOD_HEAVY);
	else if (sent > 1)
		command_add_flood(si, FLOOD_MODERATE);
	logcommand(si, CMDLOG_ADMIN, "SENDALL: \2%s\2 (%u/%u sent)", m, sent, tried);
	command_success_nodata(si, ngettext(N_("The memo has been successfully sent to \2%u\2 account."),
	                                    N_("The memo has been successfully sent to \2%u\2 accounts."),
	                                    sent), sent);
	return;
}

static struct command ms_sendall = {
	.name           = "SENDALL",
	.desc           = N_("Sends a memo to all accounts."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &ms_cmd_sendall,
	.help           = { .path = "memoserv/sendall" },
};

static void
mod_init(struct module *const restrict m)
{
        MODULE_TRY_REQUEST_SYMBOL(m, maxmemos, "memoserv/main", "maxmemos")

        service_named_bind_command("memoserv", &ms_sendall);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_sendall);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/sendall", MODULE_UNLOAD_CAPABILITY_OK)
