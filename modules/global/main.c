/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"global/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

/* global list struct */
struct global_ {
	char *text;
};

service_t *globsvs = NULL;

mowgli_list_t gs_conftable;

static void gs_cmd_global(sourceinfo_t *si, const int parc, char *parv[]);
static void gs_cmd_help(sourceinfo_t *si, const int parc, char *parv[]);

command_t gs_help = { "HELP", N_("Displays contextual help information."),
		      PRIV_GLOBAL, 1, gs_cmd_help, { .path = "help" } };
command_t gs_global = { "GLOBAL", N_("Sends a global notice."),
			PRIV_GLOBAL, 1, gs_cmd_global, { .path = "gservice/global" } };

/* *INDENT-ON* */

/* HELP <command> [params] */
static void gs_cmd_help(sourceinfo_t *si, const int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_help(si, si->service->commands);
		command_success_nodata(si, "For more specific help use \2HELP \37command\37\2.");

		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, si->service->commands);
}

/* GLOBAL <parameters>|SEND|CLEAR */
static void gs_cmd_global(sourceinfo_t *si, const int parc, char *parv[])
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

		/* destroy the list we made */
		MOWGLI_ITER_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			mowgli_node_delete(n, &globlist);
			mowgli_node_free(n);
			free(global->text);
			mowgli_heap_free(glob_heap, global);
		}

		mowgli_heap_destroy(glob_heap);
		glob_heap = NULL;
		free(sender);
		sender = NULL;

		command_success_nodata(si, "The pending message has been deleted.");

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
			/* log everything */
			logcommand(si, CMDLOG_ADMIN, "GLOBAL: \2%s\2", global->text);
		}
		logcommand(si, CMDLOG_ADMIN, "GLOBAL: (\2%zu\2 lines sent)", MOWGLI_LIST_LENGTH(&globlist));

		/* destroy the list we made */
		MOWGLI_ITER_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			mowgli_node_delete(n, &globlist);
			mowgli_node_free(n);
			free(global->text);
			mowgli_heap_free(glob_heap, global);
		}

		mowgli_heap_destroy(glob_heap);
		glob_heap = NULL;
		free(sender);
		sender = NULL;

		command_success_nodata(si, "The global notice has been sent.");

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

	command_success_nodata(si,
		"Stored text to be sent as line %zu. Use \2GLOBAL SEND\2 "
		"to send message, \2GLOBAL CLEAR\2 to delete the pending message, " "or \2GLOBAL\2 to store additional lines.", MOWGLI_LIST_LENGTH(&globlist));
}

void _modinit(module_t *m)
{
	globsvs = service_add("global", NULL, &gs_conftable);

	service_bind_command(globsvs, &gs_global);
	service_named_bind_command("operserv", &gs_global);

	service_bind_command(globsvs, &gs_help);
}

void _moddeinit(void)
{
	service_unbind_command(globsvs, &gs_help);
	service_unbind_command(globsvs, &gs_global);
	service_named_unbind_command("operserv", &gs_global);

	if (globsvs != NULL)
		service_delete(globsvs);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
