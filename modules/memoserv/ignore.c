/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains code for the Memoserv IGNORE functions
 */

#include <atheme.h>

static mowgli_patricia_t *ms_ignore_cmds = NULL;

static void
ms_cmd_ignore(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR [account]"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, ms_ignore_cmds, "IGNORE");
}

static void
ms_cmd_ignore_add(struct sourceinfo *si, int parc, char *parv[])
{
	struct myuser *tmu;
	mowgli_node_t *n;
	const char *newnick;
	char *temp;

	// Arg check
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");

		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>"));
		return;
	}

	// User attempting to ignore themself?
	if (!irccasecmp(parv[0], entity(si->smu)->name))
	{
		command_fail(si, fault_badparams, _("You cannot ignore yourself."));
		return;
	}

	// Does the target account exist?
	if (!(tmu = myuser_find_ext(parv[0])))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, parv[0]);
		return;
	}
	newnick = entity(tmu)->name;

	// Ignore list is full
	if (si->smu->memo_ignores.count >= MAXMSIGNORES)
	{
		command_fail(si, fault_toomany, _("Your ignore list is full, please DEL an account."));
		return;
	}

	// Iterate through list, make sure target not in it, if last node append
	MOWGLI_ITER_FOREACH(n, si->smu->memo_ignores.head)
	{
		temp = (char *)n->data;

		// Already in the list
		if (!irccasecmp(temp, newnick))
		{
			command_fail(si, fault_nochange, _("Account \2%s\2 is already in your ignore list."), temp);
			return;
		}
	}

	// Add to ignore list
	temp = sstrdup(newnick);
	mowgli_node_add(temp, mowgli_node_create(), &si->smu->memo_ignores);
	logcommand(si, CMDLOG_SET, "IGNORE:ADD: \2%s\2", newnick);
	command_success_nodata(si, _("Account \2%s\2 added to your ignore list."), newnick);
	return;
}

static void
ms_cmd_ignore_del(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	char *temp;

	// Arg check
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>"));
		return;
	}

	// Iterate through list, make sure they're not in it, if last node append
	MOWGLI_ITER_FOREACH_SAFE(n, tn, si->smu->memo_ignores.head)
	{
		temp = (char *)n->data;

		// Not using list or clear, but we've found our target in the ignore list
		if (!irccasecmp(temp, parv[0]))
		{
			logcommand(si, CMDLOG_SET, "IGNORE:DEL: \2%s\2", temp);
			command_success_nodata(si, _("Account \2%s\2 removed from ignore list."), temp);
			mowgli_node_delete(n, &si->smu->memo_ignores);
			mowgli_node_free(n);
			sfree(temp);

			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 is not in your ignore list."), parv[0]);
	return;
}

static void
ms_cmd_ignore_clear(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;

	if (MOWGLI_LIST_LENGTH(&si->smu->memo_ignores) == 0)
	{
		command_fail(si, fault_nochange, _("Ignore list already empty."));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, si->smu->memo_ignores.head)
	{
		sfree(n->data);
		mowgli_node_delete(n,&si->smu->memo_ignores);
		mowgli_node_free(n);
	}

	// Let them know list is clear
	command_success_nodata(si, _("Ignore list cleared."));
	logcommand(si, CMDLOG_SET, "IGNORE:CLEAR");
	return;
}

static void
ms_cmd_ignore_list(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	unsigned int i = 1;

	// Throw in list header
	command_success_nodata(si, _("Ignore list:"));
	command_success_nodata(si, "--------------------------------");

	// Iterate through list, make sure they're not in it, if last node append
	MOWGLI_ITER_FOREACH(n, si->smu->memo_ignores.head)
	{
		command_success_nodata(si, "%u - %s", i, (char *)n->data);
		i++;
	}

	// Ignore list footer
	if (i == 1)
		command_success_nodata(si, _("List empty."));

	command_success_nodata(si, "--------------------------------");
}

static struct command ms_ignore = {
	.name           = "IGNORE",
	.desc           = N_("Ignores memos."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &ms_cmd_ignore,
	.help           = { .path = "memoserv/ignore" },
};

static struct command ms_ignore_add = {
	.name           = "ADD",
	.desc           = N_("Ignores memos from a user."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ms_cmd_ignore_add,
	.help           = { .path = "" },
};

static struct command ms_ignore_del = {
	.name           = "DEL",
	.desc           = N_("Stops ignoring memos from a user."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ms_cmd_ignore_del,
	.help           = { .path = "" },
};

static struct command ms_ignore_clear = {
	.name           = "CLEAR",
	.desc           = N_("Clears your memo ignore list."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ms_cmd_ignore_clear,
	.help           = { .path = "" },
};

static struct command ms_ignore_list = {
	.name           = "LIST",
	.desc           = N_("Shows all users you are ignoring memos from."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ms_cmd_ignore_list,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "memoserv/main")

	if (! (ms_ignore_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&ms_ignore_add, ms_ignore_cmds);
	(void) command_add(&ms_ignore_del, ms_ignore_cmds);
	(void) command_add(&ms_ignore_clear, ms_ignore_cmds);
	(void) command_add(&ms_ignore_list, ms_ignore_cmds);

	(void) service_named_bind_command("memoserv", &ms_ignore);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("memoserv", &ms_ignore);

	(void) mowgli_patricia_destroy(ms_ignore_cmds, &command_delete_trie_cb, ms_ignore_cmds);
}

SIMPLE_DECLARE_MODULE_V1("memoserv/ignore", MODULE_UNLOAD_CAPABILITY_OK)
