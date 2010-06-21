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

list_t gs_cmdtree;
list_t *os_cmdtree;
list_t gs_helptree;
list_t *os_helptree;
list_t gs_conftable;

static void gs_cmd_global(sourceinfo_t *si, const int parc, char *parv[]);
static void gs_cmd_help(sourceinfo_t *si, const int parc, char *parv[]);

command_t gs_help = { "HELP", N_("Displays contextual help information."),
		      PRIV_GLOBAL, 1, gs_cmd_help };
command_t gs_global = { "GLOBAL", N_("Sends a global notice."),
			PRIV_GLOBAL, 1, gs_cmd_global };

/* *INDENT-ON* */

/* HELP <command> [params] */
static void gs_cmd_help(sourceinfo_t *si, const int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_help(si, &gs_cmdtree);
		command_success_nodata(si, "For more specific help use \2HELP \37command\37\2.");

		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, &gs_helptree);
}

/* GLOBAL <parameters>|SEND|CLEAR */
static void gs_cmd_global(sourceinfo_t *si, const int parc, char *parv[])
{
	static BlockHeap *glob_heap = NULL;
	struct global_ *global;
	static list_t globlist;
	node_t *n, *tn;
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
		LIST_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			node_del(n, &globlist);
			node_free(n);
			free(global->text);
			BlockHeapFree(glob_heap, global);
		}

		BlockHeapDestroy(glob_heap);
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
		LIST_FOREACH(n, globlist.head)
		{
			global = (struct global_ *)n->data;

			snprintf(buf, sizeof buf, "[Network Notice] %s%s%s",
					isfirst ? get_source_name(si) : "",
					isfirst ? " - " : "",
					global->text);
			/* Cannot use si->service->me here, global notices
			 * should come from global even if /os global was
			 * used. */
			notice_global_sts(globsvs.me->me, "*", buf);
			isfirst = false;
			/* log everything */
			logcommand(si, CMDLOG_ADMIN, "GLOBAL: \2%s\2", global->text);
		}
		logcommand(si, CMDLOG_ADMIN, "GLOBAL: (\2%d\2 lines sent)", LIST_LENGTH(&globlist));

		/* destroy the list we made */
		LIST_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			node_del(n, &globlist);
			node_free(n);
			free(global->text);
			BlockHeapFree(glob_heap, global);
		}

		BlockHeapDestroy(glob_heap);
		glob_heap = NULL;
		free(sender);
		sender = NULL;

		command_success_nodata(si, "The global notice has been sent.");

		return;
	}

	if (!glob_heap)
		glob_heap = BlockHeapCreate(sizeof(struct global_), 5);

	if (!sender)
		sender = sstrdup(get_source_name(si));

	if (irccasecmp(sender, get_source_name(si)))
	{
		command_fail(si, fault_noprivs, _("There is already a GLOBAL in progress by \2%s\2."), sender);
		return;
	}

	global = BlockHeapAlloc(glob_heap);

	global->text = sstrdup(params);

	n = node_create();
	node_add(global, n, &globlist);

	command_success_nodata(si,
		"Stored text to be sent as line %d. Use \2GLOBAL SEND\2 "
		"to send message, \2GLOBAL CLEAR\2 to delete the pending message, " "or \2GLOBAL\2 to store additional lines.", globlist.count);
}

/* main services client routine */
static void gservice(sourceinfo_t *si, int parc, char *parv[])
{
	char orig[BUFSIZE];
	char *cmd;
        char *text;

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
        text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

	command_exec_split(si->service, si, cmd, text, &gs_cmdtree);
}

static void global_config_ready(void *unused)
{
	globsvs.nick = globsvs.me->nick;
	globsvs.user = globsvs.me->user;
	globsvs.host = globsvs.me->host;
	globsvs.real = globsvs.me->real;
}

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	hook_add_event("config_ready");
	hook_add_config_ready(global_config_ready);

	globsvs.me = service_add("global", gservice, &gs_cmdtree, &gs_conftable);

	command_add(&gs_global, &gs_cmdtree);

	if (os_cmdtree)
		command_add(&gs_global, os_cmdtree);

	if (os_helptree)
		help_addentry(os_helptree, "GLOBAL", "help/gservice/global", NULL);

	help_addentry(&gs_helptree, "HELP", "help/help", NULL);
	help_addentry(&gs_helptree, "GLOBAL", "help/gservice/global", NULL);

	command_add(&gs_help, &gs_cmdtree);
}

void _moddeinit(void)
{
	if (globsvs.me)
	{
		globsvs.nick = NULL;
		globsvs.user = NULL;
		globsvs.host = NULL;
		globsvs.real = NULL;
		service_delete(globsvs.me);
		globsvs.me = NULL;
	}

	command_delete(&gs_global, &gs_cmdtree);

	if (os_cmdtree)
		command_delete(&gs_global, os_cmdtree);

	if (os_helptree)
		help_delentry(os_helptree, "GLOBAL");

	help_delentry(&gs_helptree, "GLOBAL");
	help_delentry(&gs_helptree, "HELP");

	command_delete(&gs_help, &gs_cmdtree);

	hook_del_config_ready(global_config_ready);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
