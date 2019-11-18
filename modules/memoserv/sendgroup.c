/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the Memoserv SENDGROUP function
 */

#include <atheme.h>
#include "../groupserv/groupserv.h"

static unsigned int *maxmemos;

static void
ms_cmd_sendgroup(struct sourceinfo *si, int parc, char *parv[])
{
	// misc structs etc
	struct myuser *tmu;
	mowgli_node_t *n, *tn;
	struct mymemo *memo;
	struct mygroup *mg;
	unsigned int sent = 0, tried = 0;
	bool ignored, operoverride = false;
	struct service *memoserv;

	// Grab args
	char *target = parv[0];
	char *m = parv[1];

	// Arg validation
	if (!target || !m)
	{
		command_fail(si, fault_needmoreparams,
			STR_INSUFFICIENT_PARAMS, "SENDGROUP");

		command_fail(si, fault_needmoreparams,
			_("Syntax: SENDGROUP <group> <memo>"));

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

	mg = group(myentity_find(target));
	if (mg == NULL)
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	si->smu->memo_ratelimit_num++;
	si->smu->memo_ratelimit_time = CURRTIME;

	MOWGLI_ITER_FOREACH(tn, mg->acs.head)
	{
		struct groupacs *ga = (struct groupacs *) tn->data;
		tmu = user(ga->mt);

		if (!(ga->flags & GA_MEMOS) || tmu == NULL || tmu == si->smu)
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
		snprintf(memo->text, sizeof memo->text, "%s %s", entity(mg)->name, m);

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
	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "SENDGROUP: to \2%s\2 (%u/%u sent) (oper override)", entity(mg)->name, sent, tried);
	else
		logcommand(si, CMDLOG_SET, "SENDGROUP: to \2%s\2 (%u/%u sent)", entity(mg)->name, sent, tried);

	command_success_nodata(si, ngettext(N_("The memo has been successfully sent to \2%u\2 member on \2%s\2."),
	                                    N_("The memo has been successfully sent to \2%u\2 members on \2%s\2."),
	                                    sent), sent, entity(mg)->name);
	return;
}

static struct command ms_sendgroup = {
	.name           = "SENDGROUP",
	.desc           = N_("Sends a memo to all members on a group."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &ms_cmd_sendgroup,
	.help           = { .path = "memoserv/sendgroup" },
};

static void
mod_init(struct module *const restrict m)
{
        MODULE_TRY_REQUEST_SYMBOL(m, maxmemos, "memoserv/main", "maxmemos")

        service_named_bind_command("memoserv", &ms_sendgroup);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_sendgroup);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/sendgroup", MODULE_UNLOAD_CAPABILITY_OK)
