/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the main() routine.
 */

#include <atheme.h>

// global list struct
struct global_ {
	char *text;
};

static struct service *globsvs = NULL;

static void
gs_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	if (parv[0])
	{
		(void) help_display(si, si->service, parv[0], si->service->commands);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_help(si, si->service->commands);
	(void) help_display_moreinfo(si, si->service, NULL);
	(void) help_display_suffix(si);
}

static void
gs_cmd_global(struct sourceinfo *si, const int parc, char *parv[])
{
	static mowgli_heap_t *glob_heap = NULL;
	struct global_ *global;
	static mowgli_list_t globlist;
	mowgli_node_t *n, *tn;
	char *params = parv[0];
	static char *sender = NULL;
	bool isfirst;
	char buf[BUFSIZE];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GLOBAL");
		command_fail(si, fault_needmoreparams, _("Syntax: GLOBAL <parameters>|SEND|CLEAR"));
		return;
	}

	if (!strcasecmp("CLEAR", params))
	{
		if (!globlist.count)
		{
			command_fail(si, fault_nochange, _("No message to clear."));
			return;
		}

		// destroy the list we made
		MOWGLI_ITER_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			mowgli_node_delete(n, &globlist);
			mowgli_node_free(n);
			sfree(global->text);
			mowgli_heap_free(glob_heap, global);
		}

		mowgli_heap_destroy(glob_heap);
		glob_heap = NULL;
		sfree(sender);
		sender = NULL;

		command_success_nodata(si, _("The pending message has been deleted."));

		return;
	}

	if (!strcasecmp("SEND", params))
	{
		if (!globlist.count)
		{
			command_fail(si, fault_nosuch_target, _("No message to send."));
			return;
		}

		isfirst = true;
		MOWGLI_ITER_FOREACH(n, globlist.head)
		{
			global = (struct global_ *)n->data;

			snprintf(buf, sizeof buf, "[Network Notice] %s%s%s",
					isfirst ? get_source_name(si) : "",
					isfirst ? " - " : "",
					global->text);

			/* Cannot use si->service->me here, global notices
			 * should come from global even if /os global was
			 * used. */
			notice_global_sts(globsvs->me, "*", buf);
			isfirst = false;

			// log everything
			logcommand(si, CMDLOG_ADMIN, "GLOBAL: \2%s\2", global->text);
		}
		logcommand(si, CMDLOG_ADMIN, "GLOBAL: (\2%zu\2 lines sent)", MOWGLI_LIST_LENGTH(&globlist));

		// destroy the list we made
		MOWGLI_ITER_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			mowgli_node_delete(n, &globlist);
			mowgli_node_free(n);
			sfree(global->text);
			mowgli_heap_free(glob_heap, global);
		}

		mowgli_heap_destroy(glob_heap);
		glob_heap = NULL;
		sfree(sender);
		sender = NULL;

		command_success_nodata(si, _("The global notice has been sent."));

		return;
	}

	if (!strcasecmp("LIST", params))
	{
		if (!globlist.count)
		{
			command_fail(si, fault_nosuch_target, _("No messages to list."));
			return;
		}

		isfirst = true;
		MOWGLI_ITER_FOREACH(n, globlist.head)
		{
			global = (struct global_ *)n->data;

			snprintf(buf, sizeof buf, "[Network Notice] %s%s%s",
					isfirst ? get_source_name(si) : "",
					isfirst ? " - " : "",
					global->text);

			/* Use command_success_nodata here, we only want to send
			 * to the person running the command.
			 */
			command_success_nodata(si, "%s", buf);
			isfirst = false;
		}
		logcommand(si, CMDLOG_ADMIN, "GLOBAL:LIST");

		command_success_nodata(si, _("End of list."));

		return;
	}

	if (!glob_heap)
		glob_heap = mowgli_heap_create(sizeof(struct global_), 5, BH_NOW);

	if (!sender)
		sender = sstrdup(get_source_name(si));

	if (irccasecmp(sender, get_source_name(si)))
	{
		command_fail(si, fault_noprivs, _("There is already a GLOBAL in progress by \2%s\2."), sender);
		return;
	}

	global = mowgli_heap_alloc(glob_heap);

	global->text = sstrdup(params);

	n = mowgli_node_create();
	mowgli_node_add(global, n, &globlist);

	command_success_nodata(si, _("Stored text to be sent as line %zu. Use \2GLOBAL SEND\2 to send the message, "
	                             "\2GLOBAL CLEAR\2 to delete the pending message, \2GLOBAL LIST\2 to preview "
	                             "what will be sent, or \2GLOBAL\2 to store additional lines."),
	                             MOWGLI_LIST_LENGTH(&globlist));
}

static struct command gs_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = PRIV_GLOBAL,
	.maxparc        = 1,
	.cmd            = &gs_cmd_help,
	.help           = { .path = "help" },
};

static struct command gs_global = {
	.name           = "GLOBAL",
	.desc           = N_("Sends a global notice."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 1,
	.cmd            = &gs_cmd_global,
	.help           = { .path = "gservice/global" },
};

static void
mod_init(struct module *const restrict m)
{
	if (! (globsvs = service_add("global", NULL)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) service_bind_command(globsvs, &gs_help);
	(void) service_bind_command(globsvs, &gs_global);

	(void) service_named_bind_command("operserv", &gs_global);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("operserv", &gs_global);

	(void) service_delete(globsvs);
}

SIMPLE_DECLARE_MODULE_V1("global/main", MODULE_UNLOAD_CAPABILITY_OK)
