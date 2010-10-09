/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * OperServ NOOP command.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/noop", true, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

typedef struct noop_ noop_t;

struct noop_ {
	char *target;
	char *added_by;
	char *reason;
};

mowgli_list_t noop_hostmask_list;
mowgli_list_t noop_server_list;

static void os_cmd_noop(sourceinfo_t *si, int parc, char *parv[]);
static void noop_kill_users(void *dummy);
static void check_quit(user_t *u);
static void check_user(user_t *u);
static mowgli_list_t noop_kill_queue;

command_t os_noop = { "NOOP", N_("Restricts IRCop access."), PRIV_NOOP, 4, os_cmd_noop, { .path = "oservice/noop" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_noop);
	hook_add_event("user_oper");
	hook_add_user_oper(check_user);
	hook_add_event("user_delete");
}

void _moddeinit()
{
	mowgli_node_t *n, *tn;

	if (MOWGLI_LIST_LENGTH(&noop_kill_queue) > 0)
	{
		/* Cannot safely delete users from here, so just forget
		 * about them.
		 */
		event_delete(noop_kill_users, NULL);
		MOWGLI_ITER_FOREACH_SAFE(n, tn, noop_kill_queue.head)
		{
			mowgli_node_delete(n, &noop_kill_queue);
			mowgli_node_free(n);
		}
		hook_del_user_delete(check_quit);
	}
	service_named_unbind_command("operserv", &os_noop);
	hook_del_user_oper(check_user);
}

static void noop_kill_users(void *dummy)
{
	service_t *service;
	mowgli_node_t *n, *tn;
	user_t *u;

	hook_del_user_delete(check_quit);

	service = service_find("operserv");
	MOWGLI_ITER_FOREACH_SAFE(n, tn, noop_kill_queue.head)
	{
		u = n->data;
		kill_user(service->me, u, "Operator access denied");
		mowgli_node_delete(n, &noop_kill_queue);
		mowgli_node_free(n);
	}
}

static void check_quit(user_t *u)
{
	mowgli_node_t *n;

	n = mowgli_node_find(u, &noop_kill_queue);
	if (n != NULL)
	{
		mowgli_node_delete(n, &noop_kill_queue);
		mowgli_node_free(n);
		if (MOWGLI_LIST_LENGTH(&noop_kill_queue) == 0)
		{
			event_delete(noop_kill_users, NULL);
			hook_del_user_delete(check_quit);
		}
	}
}

static void check_user(user_t *u)
{
	mowgli_node_t *n;
	char hostbuf[BUFSIZE];

	if (mowgli_node_find(u, &noop_kill_queue))
		return;

	snprintf(hostbuf, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->host);

	MOWGLI_ITER_FOREACH(n, noop_hostmask_list.head)
	{
		noop_t *np = n->data;

		if (!match(np->target, hostbuf))
		{
			if (MOWGLI_LIST_LENGTH(&noop_kill_queue) == 0)
			{
				event_add_once("noop_kill_users", noop_kill_users, NULL, 0);
				hook_add_user_delete(check_quit);
			}
			if (!mowgli_node_find(u, &noop_kill_queue))
				mowgli_node_add(u, mowgli_node_create(), &noop_kill_queue);
			/* Prevent them using the privs in Atheme. */
			u->flags &= ~UF_IRCOP;
			return;
		}
	}

	MOWGLI_ITER_FOREACH(n, noop_server_list.head)
	{
		noop_t *np = n->data;

		if (!match(np->target, u->server->name))
		{
			if (MOWGLI_LIST_LENGTH(&noop_kill_queue) == 0)
			{
				event_add_once("noop_kill_users", noop_kill_users, NULL, 0);
				hook_add_user_delete(check_quit);
			}
			if (!mowgli_node_find(u, &noop_kill_queue))
				mowgli_node_add(u, mowgli_node_create(), &noop_kill_queue);
			/* Prevent them using the privs in Atheme. */
			u->flags &= ~UF_IRCOP;
			return;
		}
	}
}

static noop_t *noop_find(char *target, mowgli_list_t *list)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, list->head)
	{
		noop_t *np = n->data;

		if (!match(np->target, target))
			return np;
	}

	return NULL;
}

/* NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> [reason] */
static void os_cmd_noop(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;
	noop_t *np;
	char *action = parv[0];
	enum { type_all, type_hostmask, type_server } type;
	char *mask = parv[2];
	char *reason = parv[3];

	if (parc < 1 || (strcasecmp(action, "LIST") && parc < 3))
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
		command_fail(si, fault_needmoreparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
		return;
	}
	if (parc < 2)
		type = type_all;
	else if (!strcasecmp(parv[1], "HOSTMASK"))
		type = type_hostmask;
	else if (!strcasecmp(parv[1], "SERVER"))
		type = type_server;
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOOP");
		command_fail(si, fault_badparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
		return;
	}

	if (!strcasecmp(action, "ADD"))
	{
		if (type == type_hostmask)
		{
			if ((np = noop_find(mask, &noop_hostmask_list)))
			{
				command_fail(si, fault_nochange, _("There is already a NOOP entry covering this target."));
				return;
			}

			np = smalloc(sizeof *np);

			np->target = sstrdup(mask);
			np->added_by = sstrdup(get_storage_oper_name(si));

			if (reason)
				np->reason = sstrdup(reason);
			else
				np->reason = sstrdup("Abusive operator.");

			n = mowgli_node_create();
			mowgli_node_add(np, n, &noop_hostmask_list);

			logcommand(si, CMDLOG_ADMIN, "NOOP:ADD:HOSTMASK: \2%s\2 (reason: \2%s\2)", np->target, np->reason);
			command_success_nodata(si, _("Added \2%s\2 to the hostmask NOOP list."), mask);

			return;
		}
		else if (type == type_server)
		{
			if ((np = noop_find(mask, &noop_server_list)))
			{
				command_fail(si, fault_nochange, _("There is already a NOOP entry covering this target."));
				return;
			}

			np = smalloc(sizeof *np);

			np->target = sstrdup(mask);
			np->added_by = sstrdup(get_storage_oper_name(si));

			if (reason)
				np->reason = sstrdup(reason);
			else
				np->reason = sstrdup("Abusive operator.");

			n = mowgli_node_create();
			mowgli_node_add(np, n, &noop_server_list);

			logcommand(si, CMDLOG_ADMIN, "NOOP:ADD:SERVER: \2%s\2 (reason: \2%s\2)", np->target, np->reason);
			command_success_nodata(si, _("Added \2%s\2 to the server NOOP list."), mask);

			return;
		}
	}
	else if (!strcasecmp(action, "DEL"))
	{
		if (type == type_hostmask)
		{
			if (!(np = noop_find(mask, &noop_hostmask_list)))
			{
				command_fail(si, fault_nosuch_target, _("There is no NOOP hostmask entry for this target."));
				return;
			}

			logcommand(si, CMDLOG_ADMIN, "NOOP:DEL:HOSTMASK: \2%s\2", np->target);
			command_success_nodata(si, _("Removed \2%s\2 from the hostmask NOOP list."), np->target);

			n = mowgli_node_find(np, &noop_hostmask_list);

			free(np->target);
			free(np->added_by);
			free(np->reason);

			mowgli_node_delete(n, &noop_hostmask_list);
			mowgli_node_free(n);
			free(np);

			return;
		}
		else if (type == type_server)
		{
			if (!(np = noop_find(mask, &noop_server_list)))
			{
				command_fail(si, fault_nosuch_target, _("There is no NOOP server entry for this target."));
				return;
			}

			logcommand(si, CMDLOG_ADMIN, "NOOP:DEL:SERVER: \2%s\2", np->target);
			command_success_nodata(si, _("Removed \2%s\2 from the server NOOP list."), np->target);

			n = mowgli_node_find(np, &noop_server_list);

			free(np->target);
			free(np->added_by);
			free(np->reason);

			mowgli_node_delete(n, &noop_server_list);
			mowgli_node_free(n);
			free(np);

			return;
		}
	}
	else if (!strcasecmp(action, "LIST"))
	{
		switch (type)
		{
			case type_all: 
				logcommand(si, CMDLOG_GET, "NOOP:LIST");
				break;
			case type_hostmask: 
				logcommand(si, CMDLOG_GET, "NOOP:LIST:HOSTMASK");
				break;
			case type_server: 
				logcommand(si, CMDLOG_GET, "NOOP:LIST:SERVER");
				break;
		}
		if (type == type_all || type == type_hostmask)
		{
			unsigned int i = 1;
			command_success_nodata(si, _("Hostmask NOOP list (%zu entries):"), noop_hostmask_list.count);
			command_success_nodata(si, " ");
			command_success_nodata(si, _("Entry Hostmask                        Adder                 Reason"));
			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");

			MOWGLI_ITER_FOREACH(n, noop_hostmask_list.head)
			{
				np = n->data;

				command_success_nodata(si, "%-5d %-31s %-21s %s", i, np->target, np->added_by, np->reason);
				i++;
			}

			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");
			command_success_nodata(si, _("End of Hostmask NOOP list."));
		}
		if (type == type_all || type == type_server)
		{
			unsigned int i = 1;
			command_success_nodata(si, _("Server NOOP list (%zu entries):"), noop_server_list.count);
			command_success_nodata(si, " ");
			command_success_nodata(si, _("Entry Hostmask                        Adder                 Reason"));
			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");

			MOWGLI_ITER_FOREACH(n, noop_server_list.head)
			{
				np = n->data;

				command_success_nodata(si, "%-5d %-31s %-21s %s", i, np->target, np->added_by, np->reason);
				i++;
			}

			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");
			command_success_nodata(si, _("End of Server NOOP list."));
		}
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOOP");
		command_fail(si, fault_badparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
