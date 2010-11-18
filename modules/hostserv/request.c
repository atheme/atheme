/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows requesting a vhost for a nick/account
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"hostserv/request", true, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Rizon Development Group <http://www.rizon.net>"
);

mowgli_list_t *conf_hs_table;
bool request_per_nick;

unsigned int ratelimit_count = 0;
time_t ratelimit_firsttime = 0;

static void account_drop_request(myuser_t *mu);
static void nick_drop_request(hook_user_req_t *hdata);
static void account_delete_request(myuser_t *mu);
static void hs_cmd_request(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_reject(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_activate(sourceinfo_t *si, int parc, char *parv[]);

static void write_hsreqdb(database_handle_t *db);
static void db_h_hr(database_handle_t *db, const char *type);

command_t hs_request = { "REQUEST", N_("Requests new virtual hostname for current nick."), AC_NONE, 2, hs_cmd_request, { .path = "hostserv/request" } };
command_t hs_waiting = { "WAITING", N_("Lists vhosts currently waiting for activation."), PRIV_USER_VHOST, 1, hs_cmd_waiting, { .path = "hostserv/waiting" } };
command_t hs_reject = { "REJECT", N_("Reject the requested vhost for the given nick."), PRIV_USER_VHOST, 2, hs_cmd_reject, { .path = "hostserv/reject" } };
command_t hs_activate = { "ACTIVATE", N_("Activate the requested vhost for a given nick."), PRIV_USER_VHOST, 2, hs_cmd_activate, { .path = "hostserv/activate" } };

struct hsreq_ {
	char *nick;
	char *vhost;
	time_t vhost_ts;
	char *creator;
};

typedef struct hsreq_ hsreq_t;

mowgli_list_t hs_reqlist;

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	MODULE_USE_SYMBOL(conf_hs_table, "hostserv/main", "conf_hs_table");

	hook_add_event("user_drop");
	hook_add_user_drop(account_drop_request);
	hook_add_event("nick_ungroup");
	hook_add_nick_ungroup(nick_drop_request);
	hook_add_event("myuser_delete");
	hook_add_myuser_delete(account_delete_request);
	hook_add_db_write(write_hsreqdb);

	db_register_type_handler("HR", db_h_hr);

 	service_named_bind_command("hostserv", &hs_request);
	service_named_bind_command("hostserv", &hs_waiting);
	service_named_bind_command("hostserv", &hs_reject);
	service_named_bind_command("hostserv", &hs_activate);
	add_bool_conf_item("REQUEST_PER_NICK", conf_hs_table, 0, &request_per_nick, false);
}

void _moddeinit(void)
{
	hook_del_user_drop(account_drop_request);
	hook_del_nick_ungroup(nick_drop_request);
	hook_del_myuser_delete(account_delete_request);
	hook_del_db_write(write_hsreqdb);

	db_unregister_type_handler("HR");

 	service_named_unbind_command("hostserv", &hs_request);
	service_named_unbind_command("hostserv", &hs_waiting);
	service_named_unbind_command("hostserv", &hs_reject);
	service_named_unbind_command("hostserv", &hs_activate);
	del_conf_item("REQUEST_PER_NICK", conf_hs_table);
}

static void write_hsreqdb(database_handle_t *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		hsreq_t *l = n->data;

		db_start_row(db, "HR");
		db_write_word(db, l->nick);
		db_write_word(db, l->vhost);
		db_write_time(db, l->vhost_ts);
		db_write_word(db, l->creator);
		db_commit_row(db);
	}
}

static void db_h_hr(database_handle_t *db, const char *type)
{
	const char *nick = db_sread_word(db);
	const char *vhost = db_sread_word(db);
	time_t vhost_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);

	hsreq_t *l = smalloc(sizeof(hsreq_t));
	l->nick = sstrdup(nick);
	l->vhost = sstrdup(vhost);
	l->vhost_ts = vhost_ts;
	l->creator = sstrdup(creator);
	mowgli_node_add(l, mowgli_node_create(), &hs_reqlist);
}

static void nick_drop_request(hook_user_req_t *hdata)
{
	mowgli_node_t *m;
	hsreq_t *l;

	MOWGLI_ITER_FOREACH(m, hs_reqlist.head)
	{
		l = m->data;
		if (!irccasecmp(l->nick, hdata->mn->nick))
		{
			slog(LG_REGISTER, "VHOSTREQ:DROPNICK: \2%s\2 \2%s\2", l->nick, l->vhost);

			mowgli_node_delete(m, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			return;
		}
	}
}

static void account_drop_request(myuser_t *mu)
{
	mowgli_node_t *n;
	hsreq_t *l;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, entity(mu)->name))
		{
			slog(LG_REGISTER, "VHOSTREQ:DROPACCOUNT: \2%s\2 \2%s\2", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			return;
		}
	}
}

static void account_delete_request(myuser_t *mu)
{
	mowgli_node_t *n;
	hsreq_t *l;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, entity(mu)->name))
		{
			slog(LG_REGISTER, "VHOSTREQ:EXPIRE: \2%s\2 \2%s\2", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			return;
		}
	}
}

/* REQUEST <host> */
static void hs_cmd_request(sourceinfo_t *si, int parc, char *parv[])
{
	char *host = parv[0];
	char *target;
	mynick_t *mn;
	mowgli_node_t *n;
	hsreq_t *l;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REQUEST");
		command_fail(si, fault_needmoreparams, _("Syntax: REQUEST <vhost>"));
		return;
	}

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
		ratelimit_count = 0, ratelimit_firsttime = CURRTIME;

	if (request_per_nick)
	{
		target = si->su != NULL ? si->su->nick : "?";
		mn = mynick_find(target);
		if (mn == NULL)
		{
			command_fail(si, fault_nosuch_target, _("Nick \2%s\2 is not registered."), target);
			return;
		}
		if (mn->owner != si->smu)
		{
			command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
			return;
		}
	}
	else
		target = entity(si->smu)->name;

	if (!check_vhost_validity(si, host))
		return;

	/* search for it */
	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;

		if (!irccasecmp(l->nick, target))
		{
			if (!strcmp(host, l->vhost))
			{
				command_success_nodata(si, _("You have already requested vhost \2%s\2."), host);
				return;
			}
			if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
			{
				command_fail(si, fault_toomany, _("The system is currently too busy to process your vHost request, please try again later."));
				slog(LG_INFO, "VHOSTREQUEST:THROTTLED: %s", si->su->nick);
				return;
			}
			free(l->vhost);
			l->vhost = sstrdup(host);
			l->vhost_ts = CURRTIME;;

			command_success_nodata(si, _("You have requested vhost \2%s\2."), host);
			logcommand(si, CMDLOG_REQUEST, "REQUEST: \2%s\2", host);
			if (config_options.ratelimit_uses && config_options.ratelimit_period)
				ratelimit_count++;
			return;
		}
	}

	if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("The system is currently too busy to process your vHost request, please try again later."));
		slog(LG_INFO, "VHOSTREQUEST:THROTTLED: %s", si->su->nick);
		return;
	}
	l = smalloc(sizeof(hsreq_t));
	l->nick = sstrdup(target);
	l->vhost = sstrdup(host);
	l->vhost_ts = CURRTIME;;
	l->creator = sstrdup(get_source_name(si));

	n = mowgli_node_create();
	mowgli_node_add(l, n, &hs_reqlist);

	command_success_nodata(si, _("You have requested vhost \2%s\2."), host);
	logcommand(si, CMDLOG_REQUEST, "REQUEST: \2%s\2", host);
	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;
	return;
}

/* ACTIVATE <nick> */
static void hs_cmd_activate(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick = parv[0];
	user_t *u;
	char buf[BUFSIZE];
	hsreq_t *l;
	mowgli_node_t *n, *tn;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACTIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: ACTIVATE <nick>"));
		return;
	}


	MOWGLI_ITER_FOREACH_SAFE(n, tn, hs_reqlist.head)
	{
		l = n->data;

		if (!irccasecmp(l->nick, nick))
		{
			if ((u = user_find_named(nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been approved.", l->vhost, nick);
			/* VHOSTNICK command below will generate snoop */
			logcommand(si, CMDLOG_REQUEST, "ACTIVATE: \2%s\2 for \2%s\2", l->vhost, nick);
			snprintf(buf, BUFSIZE, "%s %s", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			command_exec_split(si->service, si, request_per_nick ? "VHOSTNICK" : "VHOST", buf, si->service->commands);
			return;
		}

		if (!irccasecmp("*", nick))
		{
			if ((u = user_find_named(l->nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been approved.", l->vhost, l->nick);
			/* VHOSTNICK command below will generate snoop */
			logcommand(si, CMDLOG_REQUEST, "ACTIVATE: \2%s\2 for \2%s\2", l->vhost, l->nick);
			snprintf(buf, BUFSIZE, "%s %s", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			command_exec_split(si->service, si, request_per_nick ? "VHOSTNICK" : "VHOST", buf, si->service->commands);

			if (hs_reqlist.count == 0)
				return;
		}

	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

/* REJECT <nick> */
static void hs_cmd_reject(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick = parv[0];
	user_t *u;
	char buf[BUFSIZE];
	hsreq_t *l;
	mowgli_node_t *n, *tn;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: REJECT <nick>"));
		return;
	}


	MOWGLI_ITER_FOREACH_SAFE(n, tn, hs_reqlist.head)
	{
		service_t *svs;

		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((svs = service_find("memoserv")) != NULL)
			{
				snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", nick, l->vhost, nick);
				command_exec_split(svs, si, "SEND", buf, svs->commands);
			}
			else if ((u = user_find_named(nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->vhost, nick);
			logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2", l->vhost, nick);

			mowgli_node_delete(n, &hs_reqlist);
			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);
			return;
		}
		
		if (!irccasecmp("*", nick))
		{
			if ((svs = service_find("memoserv")) != NULL)
			{
				snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->nick, l->vhost, l->nick);
				command_exec_split(svs, si, "SEND", buf, svs->commands);
			}
			else if ((u = user_find_named(l->nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->vhost, l->nick);
			/* VHOSTNICK command below will generate snoop */
			logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2", l->vhost, l->nick);

			mowgli_node_delete(n, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			if (hs_reqlist.count == 0)
				return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

/* WAITING */
static void hs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[])
{
	hsreq_t *l;
	mowgli_node_t *n;
	char buf[BUFSIZE];
	struct tm tm;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;

		tm = *localtime(&l->vhost_ts);
		strftime(buf, BUFSIZE, "%b %d %T %Y %Z", &tm);
		command_success_nodata(si, "Nick:\2%s\2, vhost:\2%s\2 (%s - %s)",
			l->nick, l->vhost, l->creator, buf);
	}
	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "WAITING");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
