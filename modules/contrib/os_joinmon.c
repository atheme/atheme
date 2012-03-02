/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Facilitates watching what channels a user joins.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/os_joinmon", true, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.net>"
);

static void watch_user_joins(hook_channel_joinpart_t *hdata);
static void os_cmd_joinmon(sourceinfo_t *si, int parc, char *parv[]);

static void write_jmdb(database_handle_t *db);
static void db_h_jm(database_handle_t *db, const char *type);

command_t os_joinmon = { "JOINMON", N_("Monitors what channels a user is joining."), PRIV_USER_ADMIN, 3, os_cmd_joinmon, { .path = "contrib/joinmon" } };

struct joinmon_ {
	char *user;
	/* This module is Jamaican...mon. */
	time_t mon_ts;
	char *creator;
	char *reason;
};

typedef struct joinmon_ joinmon_t;

mowgli_list_t os_monlist;

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	hook_add_event("channel_join");
	hook_add_channel_join(watch_user_joins);
	hook_add_db_write(write_jmdb);

	db_register_type_handler("JM", db_h_jm);

	service_named_bind_command("operserv", &os_joinmon);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_channel_join(watch_user_joins);
	hook_del_db_write(write_jmdb);

	db_unregister_type_handler("JM");

	service_named_unbind_command("operserv", &os_joinmon);
}

static void write_jmdb(database_handle_t *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, os_monlist.head)
	{
		joinmon_t *l = n->data;

		db_start_row(db, "JM");
		db_write_word(db, l->user);
		db_write_time(db, l->mon_ts);
		db_write_word(db, l->creator);
		db_write_str(db, l->reason);
		db_commit_row(db);
	}
}

static void db_h_jm(database_handle_t *db, const char *type)
{
	const char *user = db_sread_word(db);
	time_t mon_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *reason = db_sread_str(db);

	joinmon_t *l = smalloc(sizeof(joinmon_t));
	l->user = sstrdup(user);
	l->mon_ts = mon_ts;
	l->creator = sstrdup(creator);
	l->reason = sstrdup(reason);
	mowgli_node_add(l, mowgli_node_create(), &os_monlist);
}

static void watch_user_joins(hook_channel_joinpart_t *hdata)
{
	mowgli_node_t *n;
	chanuser_t *cu = hdata->cu;
	joinmon_t *l;

	if (cu == NULL)
		return;

	if (!(cu->user->server->flags & SF_EOB))
		return;

	MOWGLI_ITER_FOREACH(n, os_monlist.head)
	{
		l = n->data;

		/* Use match so you can monitor patterns like SomeBot* or
		 * t???h?????1
		 */
		if (!match(l->user, cu->user->nick))
		{
			/* Use LG_INFO because there's really no better logtype and creating
			 * one just for this module (ie: having to put stuff in core) is
			 * kind of stupid. Give it it's own logtype if logtypes are ever
			 * addable by modules.
			 */
			slog(LG_INFO, "JOINMON: \2%s\2 who matches \2%s\2 has joined \2%s\2",
					cu->user->nick, l->user, cu->chan->name);
			return;
		}
	}
}

static void os_cmd_joinmon(sourceinfo_t *si, int parc, char *parv[])
{
	char *action = parv[0];
	char *pattern = parv[1];
	char *reason = parv[2];
	mowgli_node_t *n, *tn;
	joinmon_t *l;

	if (!action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINMON");
		command_fail(si, fault_needmoreparams, _("Syntax: JOINMON ADD|DEL|LIST [parameters]"));
		return;
	}

	if (!strcasecmp("ADD", action))
	{
		if (!pattern)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINMON");
			command_fail(si, fault_needmoreparams, _("Syntax: JOINMON ADD <pattern> [reason]"));
			return;
		}

		if (si->smu == NULL)
		{
			command_fail(si, fault_noprivs, _("You are not logged in."));
			return;
		}

		/* search for it */
		MOWGLI_ITER_FOREACH(n, os_monlist.head)
		{
			l = n->data;

			if (!irccasecmp(l->user, pattern))
			{
				command_success_nodata(si, _("Pattern \2%s\2 is already being monitored."), pattern);
				return;
			}
		}

		l = smalloc(sizeof(joinmon_t));
		l->user = sstrdup(pattern);
		l->mon_ts = CURRTIME;;
		l->creator = sstrdup(get_source_name(si));

		if (reason)
		{
			l->reason = sstrdup(reason);
			logcommand(si, CMDLOG_ADMIN, "JOINMON:ADD: \2%s\2 (Reason: \2%s\2)", pattern, reason);
		}
		else
		{
			l->reason = _("None");
			logcommand(si, CMDLOG_ADMIN, "JOINMON:ADD: \2%s\2", pattern);
		}

		n = mowgli_node_create();
		mowgli_node_add(l, n, &os_monlist);

		command_success_nodata(si, _("\2%s\2 is now being monitored."), pattern);
		return;
	}
	else if (!strcasecmp("DEL", action))
	{
		if (!pattern)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JOINMON");
			command_fail(si, fault_needmoreparams, _("Syntax: JOINMON DEL <pattern>"));
			return;
		}

		MOWGLI_ITER_FOREACH_SAFE(n, tn, os_monlist.head)
		{
			l = n->data;

			if (!irccasecmp(l->user, pattern))
			{
				logcommand(si, CMDLOG_ADMIN, "JOINMON:DEL: \2%s\2", l->user);

				mowgli_node_delete(n, &os_monlist);

				free(l->user);
				free(l->creator);
				free(l->reason);
				free(l);

				return;
			}
		}
		command_success_nodata(si, _("Pattern \2%s\2 not found in joinmon database."), pattern);
		return;
	}
	else if (!strcasecmp("LIST", action))
	{
		char buf[BUFSIZE];
		struct tm tm;

		MOWGLI_ITER_FOREACH(n, os_monlist.head)
		{
			l = n->data;

			tm = *localtime(&l->mon_ts);
			strftime(buf, BUFSIZE, TIME_FORMAT, &tm);
			command_success_nodata(si, "Pattern: \2%s\2, Reason: \2%s\2 (%s - %s)",
				l->user, l->reason, l->creator, buf);
		}
		command_success_nodata(si, "End of list.");
		logcommand(si, CMDLOG_GET, "JOINMON:LIST");
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "JOINMON");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
