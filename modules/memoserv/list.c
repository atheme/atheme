/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the Memoserv LIST function
 */

#include <atheme.h>

static void
ms_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	// Misc structs etc
	struct mymemo *memo;
	mowgli_node_t *n;
	unsigned int i = 0;
	char strfbuf[BUFSIZE];
	struct tm *tm;
	char line[512];
	char chan[CHANNELLEN + 1];
	char *p;

	command_success_nodata(si, ngettext(N_("You have %zu memo (%u new)."),
					    N_("You have %zu memos (%u new)."),
					    si->smu->memos.count), si->smu->memos.count, si->smu->memoct_new);

	// Check to see if any memos
	if (!si->smu->memos.count)
		return;

	// Go to listing memos
	command_success_nodata(si, " ");

	MOWGLI_ITER_FOREACH(n, si->smu->memos.head)
	{
		i++;
		memo = (struct mymemo *)n->data;
		tm = localtime(&memo->sent);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		snprintf(line, sizeof line, _("- %u From: %s Sent: %s"),
				i, memo->sender, strfbuf);
		if (memo->status & MEMO_CHANNEL && *memo->text == '#')
		{
			mowgli_strlcat(line, " ", sizeof line);
			mowgli_strlcat(line, _("To:"), sizeof line);
			mowgli_strlcat(line, " ", sizeof line);
			mowgli_strlcpy(chan, memo->text, sizeof chan);
			p = strchr(chan, ' ');
			if (p != NULL)
				*p = '\0';
			mowgli_strlcat(line, chan, sizeof line);
		}
		if (!(memo->status & MEMO_READ))
		{
			mowgli_strlcat(line, " ", sizeof line);
			mowgli_strlcat(line, _("[unread]"), sizeof line);
		}
		command_success_nodata(si, "%s", line);
	}

	return;
}

static struct command ms_list = {
	.name           = "LIST",
	.desc           = N_("Lists all of your memos."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 0,
	.cmd            = &ms_cmd_list,
	.help           = { .path = "memoserv/list" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "memoserv/main")

        service_named_bind_command("memoserv", &ms_list);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_list);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/list", MODULE_UNLOAD_CAPABILITY_OK)
