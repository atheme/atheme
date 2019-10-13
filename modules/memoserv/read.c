/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the Memoserv READ function
 */

#include <atheme.h>

#define MAX_READ_AT_ONCE 5

static void
ms_cmd_read(struct sourceinfo *si, int parc, char *parv[])
{
	// Misc structs etc
	struct myuser *tmu;
	struct mymemo *memo, *receipt;
	mowgli_node_t *n;
	unsigned int i = 1, memonum = 0, numread = 0;
	char strfbuf[BUFSIZE];
	struct tm *tm;
	bool readnew;

	// Grab arg
	char *arg1 = parv[0];

	if (!arg1)
	{
		command_fail(si, fault_needmoreparams,
			STR_INSUFFICIENT_PARAMS, "READ");

		command_fail(si, fault_needmoreparams, _("Syntax: READ <memo number>"));
		return;
	}

	// Check to see if any memos
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("You have no memos."));
		return;
	}

	readnew = !strcasecmp(arg1, "NEW");
	if (!readnew && (! string_to_uint(arg1, &memonum) || ! memonum))
	{
		command_fail(si, fault_badparams, _("Invalid message index."));
		return;
	}

	// Check to see if memonum is greater than memocount
	if (memonum > si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("Invalid message index."));
		return;
	}

	// Go to reading memos
	MOWGLI_ITER_FOREACH(n, si->smu->memos.head)
	{
		memo = (struct mymemo *)n->data;
		if (i == memonum || (readnew && !(memo->status & MEMO_READ)))
		{
			tm = localtime(&memo->sent);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

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
					// If they have an account, their inbox is not full and they aren't memoserv
					if ( (tmu != NULL) && (tmu->memos.count < me.mdlimit) && strcasecmp(si->service->nick, memo->sender))
					{
						// Malloc and populate memo struct
						receipt = smalloc(sizeof *receipt);
						receipt->sent = CURRTIME;
						mowgli_strlcpy(receipt->sender, si->service->nick, sizeof receipt->sender);
						snprintf(receipt->text, sizeof receipt->text, "%s has read a memo from you sent at %s", entity(si->smu)->name, strfbuf);

						// Attach to their linked list
						n = mowgli_node_create();
						mowgli_node_add(receipt, n, &tmu->memos);
						tmu->memoct_new++;
					}
				}
			}

			command_success_nodata(si, _("\2Memo %u - Sent by %s, %s\2"), i, memo->sender, strfbuf);
			command_success_nodata(si, "----------------------------------------------------------------");
			command_success_nodata(si, "%s", memo->text);
			command_success_nodata(si, "----------------------------------------------------------------");

			if (!readnew)
				return;
			if (++numread >= MAX_READ_AT_ONCE && si->smu->memoct_new > 0)
			{
				command_success_nodata(si, _("Stopping command after %u memos."), numread);
				return;
			}
		}
		i++;
	}

	if (readnew && numread == 0)
		command_fail(si, fault_nosuch_key, _("You have no new memos."));
	else if (readnew)
		command_success_nodata(si, ngettext(N_("Read %u memo."),
		                                    N_("Read %u memos."),
		                                    numread), numread);
}

static struct command ms_read = {
	.name           = "READ",
	.desc           = N_("Reads a memo."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &ms_cmd_read,
	.help           = { .path = "memoserv/read" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "memoserv/main")

        service_named_bind_command("memoserv", &ms_read);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_read);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/read", MODULE_UNLOAD_CAPABILITY_OK)
