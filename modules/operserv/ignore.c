/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Patrick Fish, et al.
 *
 * This file contains functionality which implements the OService IGNORE command.
 */

#include <atheme.h>

static mowgli_patricia_t *os_ignore_cmds = NULL;

mowgli_list_t svs_ignore_list;

static void
os_cmd_ignore(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	if (parc < 1)
	{
		(void) command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		(void) command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>"));
		return;
	}

	(void) subcommand_dispatch_simple(si->service, si, parc, parv, os_ignore_cmds, "IGNORE");
}

static void
os_cmd_ignore_add(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
        char *target = parv[0];
	char *reason = parv[1];
	struct svsignore *svsignore;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD <mask> <reason>"));
		return;
	}

	if (!validhostmask(target))
	{
		command_fail(si, fault_badparams, _("Invalid host mask, %s"), target);
		return;
	}

	// Are we already ignoring this mask?
	MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (struct svsignore *)n->data;

		// We're here
		if (!strcasecmp(svsignore->mask, target))
		{
			command_fail(si, fault_nochange, _("The mask \2%s\2 already exists on the services ignore list."), svsignore->mask);
			return;
		}
	}

	svsignore = svsignore_add(target, reason);
	svsignore->setby = sstrdup(get_storage_oper_name(si));
	svsignore->settime = CURRTIME;

	command_success_nodata(si, _("\2%s\2 has been added to the services ignore list."), target);

	logcommand(si, CMDLOG_ADMIN, "IGNORE:ADD: \2%s\2 (reason: \2%s\2)", target, reason);
	wallops("\2%s\2 added a services ignore for \2%s\2 (%s).", get_oper_name(si), target, reason);

	return;
}

static void
os_cmd_ignore_del(struct sourceinfo *si, int parc, char *parv[])
{
	char *target = parv[0];
	mowgli_node_t *n, *tn;
	struct svsignore *svsignore;

	if (target == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <mask>"));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (struct svsignore *)n->data;

		if (!strcasecmp(svsignore->mask,target))
		{
			command_success_nodata(si, _("\2%s\2 has been removed from the services ignore list."), svsignore->mask);

			svsignore_delete(svsignore);

			wallops("\2%s\2 removed \2%s\2 from the services ignore list.", get_oper_name(si), target);
			logcommand(si, CMDLOG_ADMIN, "IGNORE:DEL: \2%s\2", target);

			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 was not found on the services ignore list."), target);
	return;
}

static void
os_cmd_ignore_clear(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n, *tn;
	struct svsignore *svsignore;

	if (MOWGLI_LIST_LENGTH(&svs_ignore_list) == 0)
	{
		command_success_nodata(si, _("Services ignore list is empty."));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, svs_ignore_list.head)
	{
		svsignore = (struct svsignore *)n->data;

		command_success_nodata(si, _("\2%s\2 has been removed from the services ignore list."), svsignore->mask);
		mowgli_node_delete(n,&svs_ignore_list);
		mowgli_node_free(n);
		sfree(svsignore->mask);
		sfree(svsignore->setby);
		sfree(svsignore->reason);

	}

	command_success_nodata(si, _("Services ignore list has been wiped!"));

	wallops("\2%s\2 wiped the services ignore list.", get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "IGNORE:CLEAR");

	return;
}

static void
os_cmd_ignore_list(struct sourceinfo *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	unsigned int i = 1;
	struct svsignore *svsignore;
	char strfbuf[BUFSIZE];
	struct tm *tm;

	if (MOWGLI_LIST_LENGTH(&svs_ignore_list) == 0)
	{
		command_success_nodata(si, _("The services ignore list is empty."));
		return;
	}

	command_success_nodata(si, _("Current Ignore list entries:"));
	command_success_nodata(si, "----------------------------------------------------------------");

	MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (struct svsignore *)n->data;

		tm = localtime(&svsignore->settime);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, tm);

		command_success_nodata(si, _("%u: %s by %s on %s (Reason: %s)"), i, svsignore->mask, svsignore->setby, strfbuf, svsignore->reason);
		i++;
	}

	command_success_nodata(si, "----------------------------------------------------------------");
	logcommand(si, CMDLOG_ADMIN, "IGNORE:LIST");
}

static struct command os_ignore = {
	.name           = "IGNORE",
	.desc           = N_("Ignore a mask from services."),
	.access         = PRIV_ADMIN,
	.maxparc        = 3,
	.cmd            = &os_cmd_ignore,
	.help           = { .path = "oservice/ignore" },
};

static struct command os_ignore_add = {
	.name           = "ADD",
	.desc           = N_("Add services ignore"),
	.access         = PRIV_ADMIN,
	.maxparc        = 2,
	.cmd            = &os_cmd_ignore_add,
	.help           = { .path = "" },
};

static struct command os_ignore_del = {
	.name           = "DEL",
	.desc           = N_("Delete services ignore"),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &os_cmd_ignore_del,
	.help           = { .path = "" },
};

static struct command os_ignore_list = {
	.name           = "LIST",
	.desc           = N_("List services ignores"),
	.access         = PRIV_ADMIN,
	.maxparc        = 0,
	.cmd            = &os_cmd_ignore_list,
	.help           = { .path = "" },
};

static struct command os_ignore_clear = {
	.name           = "CLEAR",
	.desc           = N_("Clear all services ignores"),
	.access         = PRIV_ADMIN,
	.maxparc        = 0,
	.cmd            = &os_cmd_ignore_clear,
	.help           = { .path = "" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "operserv/main")

	if (! (os_ignore_cmds = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) command_add(&os_ignore_add, os_ignore_cmds);
	(void) command_add(&os_ignore_del, os_ignore_cmds);
	(void) command_add(&os_ignore_clear, os_ignore_cmds);
	(void) command_add(&os_ignore_list, os_ignore_cmds);

	(void) service_named_bind_command("operserv", &os_ignore);

	use_svsignore++;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	use_svsignore--;

	(void) service_named_unbind_command("operserv", &os_ignore);

	(void) mowgli_patricia_destroy(os_ignore_cmds, &command_delete_trie_cb, os_ignore_cmds);
}

SIMPLE_DECLARE_MODULE_V1("operserv/ignore", MODULE_UNLOAD_CAPABILITY_OK)
