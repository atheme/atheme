/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This file contains code for the Memoserv DELETE function
 */

#include <atheme.h>

static void
ms_cmd_delete(struct sourceinfo *si, int parc, char *parv[])
{
	// Misc structs etc
	mowgli_node_t *n, *tn;
	unsigned int i = 0, delcount = 0, memonum = 0;
	unsigned int deleteall = 0, deleteold = 0;
	struct mymemo *memo;
	char *errptr = NULL;

	// We only take 1 arg, and we ignore all others
	char *arg1 = parv[0];

	// Does the arg exist?
	if (!arg1)
	{
		command_fail(si, fault_needmoreparams,
			STR_INSUFFICIENT_PARAMS, "DELETE");

		command_fail(si, fault_needmoreparams, _("Syntax: DELETE ALL|OLD|message id"));
		return;
	}

	// Do we have any memos?
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nochange, _("You have no memos to delete."));
		return;
	}

	// Do we want to delete all memos?
	if (!strcasecmp("all",arg1))
	{
		deleteall = 1;
	}
	else if (!strcasecmp("old",arg1))
	{
		deleteold = 1;
	}
	else
	{
		memonum = strtoul(arg1, &errptr, 10);

		// Make sure they didn't slip us an alphabetic index
		if (!memonum || (errptr && *errptr))
		{
			command_fail(si, fault_badparams, _("Invalid message index."));
			return;
		}

		// If int, does that index exist? And do we have something to delete?
		if (memonum > si->smu->memos.count)
		{
			command_fail(si, fault_nosuch_key, _("The specified memo doesn't exist."));
			return;
		}
	}

	delcount = 0;

	// Iterate through memos, doing deletion
	MOWGLI_ITER_FOREACH_SAFE(n, tn, si->smu->memos.head)
	{
		i++;
		memo = (struct mymemo *) n->data;

		if (i == memonum || deleteall ||
				(deleteold && memo->status & MEMO_READ))
		{
			delcount++;

			if (!(memo->status & MEMO_READ))
				si->smu->memoct_new--;

			// Free to node pool, remove from chain
			mowgli_node_delete(n, &si->smu->memos);
			mowgli_node_free(n);

			sfree(memo);
		}

	}

	command_success_nodata(si, ngettext(N_("%u memo deleted."), N_("%u memos deleted."), delcount), delcount);

	return;
}

static struct command ms_delete = {
	.name           = "DELETE",
	.desc           = N_("Deletes memos."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ms_cmd_delete,
	.help           = { .path = "memoserv/delete" },
};

static struct command ms_del = {
	.name           = "DEL",
	.desc           = N_("Alias for DELETE"),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ms_cmd_delete,
	.help           = { .path = "memoserv/delete" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "memoserv/main")

        service_named_bind_command("memoserv", &ms_delete);
	service_named_bind_command("memoserv", &ms_del);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("memoserv", &ms_delete);
	service_named_unbind_command("memoserv", &ms_del);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/delete", MODULE_UNLOAD_CAPABILITY_OK)
