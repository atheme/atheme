/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the Memoserv SEND function
 */

#include <atheme.h>

static unsigned int *maxmemos;

static void
ms_cmd_send(struct sourceinfo *si, int parc, char *parv[])
{
	// misc structs etc
	struct user *tu;
	struct myuser *tmu;
	mowgli_node_t *n;
	struct mymemo *memo;
	struct command *cmd;
	struct service *memoserv;

	// Grab args
	char *target = parv[0];
	char *m = parv[1];

	// Arg validation
	if (!target || !m)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SEND");
		command_fail(si, fault_needmoreparams, _("Syntax: SEND <user> <memo>"));
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

	memoserv = service_find("memoserv");
	if (memoserv == NULL)
		memoserv = si->service;

	if (*target != '#' && *target != '!')
	{
		// See if target is valid
		if (!(tmu = myuser_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target,
				STR_IS_NOT_REGISTERED, target);

			return;
		}

		si->smu->memo_ratelimit_num++;
		si->smu->memo_ratelimit_time = CURRTIME;

		// Does the user allow memos? --pfish
		if (tmu->flags & MU_NOMEMO)
		{
			command_fail(si, fault_noprivs, _("\2%s\2 does not wish to receive memos."), target);
			return;
		}

		// Check to make sure target inbox not full
		if (tmu->memos.count >= *maxmemos)
		{
			command_fail(si, fault_toomany, _("%s's inbox is full"), target);
			logcommand(si, CMDLOG_SET, "failed SEND to \2%s\2 (target inbox full)", entity(tmu)->name);
			return;
		}

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
				logcommand(si, CMDLOG_SET, "failed SEND to \2%s\2 (on ignore list)", entity(tmu)->name);
				command_success_nodata(si, _("The memo has been successfully sent to \2%s\2."), target);
				return;
			}
		}
		logcommand(si, CMDLOG_SET, "SEND: to \2%s\2", entity(tmu)->name);

		// Malloc and populate struct
		memo = smalloc(sizeof *memo);
		memo->sent = CURRTIME;
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

		/* Note: do not disclose other nicks they're logged in with
		 * -- jilles
		 *
		 * Actually, I don't see the point in this at all. If they want this information,
		 * they should use WHOIS. --nenolod
		 */
		tu = user_find_named(target);
		if (tu != NULL && tu->myuser == tmu)
			command_success_nodata(si, _("%s is currently online, and you may talk directly, by sending a private message."), target);

		// Is the user online? If so, tell them about the new memo.
		if (si->su == NULL || !irccasecmp(si->su->nick, entity(si->smu)->name))
			myuser_notice(memoserv->nick, tmu, "You have a new memo from %s (%zu).", entity(si->smu)->name, MOWGLI_LIST_LENGTH(&tmu->memos));
		else
			myuser_notice(memoserv->nick, tmu, "You have a new memo from %s (nick: %s) (%zu).", entity(si->smu)->name, si->su->nick, MOWGLI_LIST_LENGTH(&tmu->memos));

		myuser_notice(si->service->nick, tmu, "To read it, type \2/msg %s READ %zu\2",
		              memoserv->disp, MOWGLI_LIST_LENGTH(&tmu->memos));

		// Tell user memo sent
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

static struct command ms_send = {
	.name           = "SEND",
	.desc           = N_("Sends a memo to a user."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &ms_cmd_send,
	.help           = { .path = "memoserv/send" },
};

static void
mod_init(struct module *const restrict m)
{
        MODULE_TRY_REQUEST_SYMBOL(m, maxmemos, "memoserv/main", "maxmemos")

        service_named_bind_command("memoserv", &ms_send);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_send);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/send", MODULE_UNLOAD_CAPABILITY_OK)
