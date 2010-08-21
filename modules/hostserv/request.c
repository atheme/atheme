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
	"hostserv/request", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Rizon Development Group <http://www.rizon.net>"
);

list_t *hs_cmdtree, *hs_helptree, *conf_hs_table, *ms_cmdtree;
bool request_per_nick;

unsigned int ratelimit_count = 0;
time_t ratelimit_firsttime = 0;

static void account_drop_request(myuser_t *mu);
static void nick_drop_request(hook_user_req_t *hdata);
static void hs_cmd_request(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_reject(sourceinfo_t *si, int parc, char *parv[]);
static void hs_cmd_activate(sourceinfo_t *si, int parc, char *parv[]);

static void write_hsreqdb(database_handle_t *db);
static void db_h_hr(database_handle_t *db, const char *type);

command_t hs_request = { "REQUEST", N_("Requests new virtual hostname for current nick."), AC_NONE, 2, hs_cmd_request };
command_t hs_waiting = { "WAITING", N_("Lists vhosts currently waiting for activation."), PRIV_USER_VHOST, 1, hs_cmd_waiting };
command_t hs_reject = { "REJECT", N_("Reject the requested vhost for the given nick."), PRIV_USER_VHOST, 2, hs_cmd_reject };
command_t hs_activate = { "ACTIVATE", N_("Activate the requested vhost for a given nick."), PRIV_USER_VHOST, 2, hs_cmd_activate };

struct hsreq_ {
	char *nick;
	char *vhost;
	time_t vhost_ts;
	char *creator;
};

typedef struct hsreq_ hsreq_t;

list_t hs_reqlist;

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	MODULE_USE_SYMBOL(hs_cmdtree, "hostserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "hostserv/main", "hs_helptree");
	MODULE_USE_SYMBOL(conf_hs_table, "hostserv/main", "conf_hs_table");
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");

	hook_add_event("user_drop");
	hook_add_user_drop(account_drop_request);
	hook_add_event("nick_ungroup");
	hook_add_nick_ungroup(nick_drop_request);
	hook_add_db_write(write_hsreqdb);

	db_register_type_handler("HR", db_h_hr);

 	command_add(&hs_request, hs_cmdtree);
	command_add(&hs_waiting, hs_cmdtree);
	command_add(&hs_reject, hs_cmdtree);
	command_add(&hs_activate, hs_cmdtree);
	help_addentry(hs_helptree, "REQUEST", "help/hostserv/request", NULL);
	help_addentry(hs_helptree, "WAITING", "help/hostserv/waiting", NULL);
	help_addentry(hs_helptree, "REJECT", "help/hostserv/reject", NULL);
	help_addentry(hs_helptree, "ACTIVATE", "help/hostserv/activate", NULL);
	add_bool_conf_item("REQUEST_PER_NICK", conf_hs_table, 0, &request_per_nick, false);
}

void _moddeinit(void)
{
	hook_del_user_drop(account_drop_request);
	hook_del_nick_ungroup(nick_drop_request);
	hook_del_db_write(write_hsreqdb);

	db_unregister_type_handler("HR");

	command_delete(&hs_request, hs_cmdtree);
	command_delete(&hs_waiting, hs_cmdtree);
	command_delete(&hs_reject, hs_cmdtree);
	command_delete(&hs_activate, hs_cmdtree);
	help_delentry(hs_helptree, "REQUEST");
	help_delentry(hs_helptree, "WAITING");
	help_delentry(hs_helptree, "REJECT");
	help_delentry(hs_helptree, "ACTIVATE");
	del_conf_item("REQUEST_PER_NICK", conf_hs_table);
}

static void write_hsreqdb(database_handle_t *db)
{
	node_t *n;

	LIST_FOREACH(n, hs_reqlist.head)
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
	node_add(l, node_create(), &hs_reqlist);
}

static void nick_drop_request(hook_user_req_t *hdata)
{
	node_t *m;
	hsreq_t *l;

	LIST_FOREACH(m, hs_reqlist.head)
	{
		l = m->data;
		if (!irccasecmp(l->nick, hdata->mn->nick))
		{
			slog(LG_REGISTER, "VHOSTREQ:DROPNICK: \2%s\2 \2%s\2", l->nick, l->vhost);

			node_del(m, &hs_reqlist);

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
	node_t *n;
	hsreq_t *l;

	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, entity(mu)->name))
		{
			slog(LG_REGISTER, "VHOSTREQ:DROPACCOUNT: \2%s\2 \2%s\2", l->nick, l->vhost);

			node_del(n, &hs_reqlist);

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
	node_t *n;
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
	LIST_FOREACH(n, hs_reqlist.head)
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

	n = node_create();
	node_add(l, n, &hs_reqlist);

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
	node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACTIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: ACTIVATE <nick>"));
		return;
	}


	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((u = user_find_named(nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been approved.", l->vhost, nick);
			/* VHOSTNICK command below will generate snoop */
			logcommand(si, CMDLOG_REQUEST, "ACTIVATE: \2%s\2 for \2%s\2", l->vhost, nick);
			snprintf(buf, BUFSIZE, "%s %s", l->nick, l->vhost);

			node_del(n, &hs_reqlist);

			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);

			command_exec_split(si->service, si, request_per_nick ? "VHOSTNICK" : "VHOST", buf, hs_cmdtree);
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
	node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: REJECT <nick>"));
		return;
	}


	LIST_FOREACH(n, hs_reqlist.head)
	{
		service_t *svs;

		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((svs = service_find("memoserv")) != NULL)
			{
				snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", nick, l->vhost, nick);
				command_exec_split(svs, si, "SEND", buf, ms_cmdtree);
			}
			else if ((u = user_find_named(nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->vhost, nick);
			logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2", l->vhost, nick);

			node_del(n, &hs_reqlist);
			free(l->nick);
			free(l->vhost);
			free(l->creator);
			free(l);
			return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

/* WAITING */
static void hs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[])
{
	hsreq_t *l;
	node_t *n;
	int x = 0;
	char buf[BUFSIZE];
	struct tm tm;

	LIST_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		x++;

		tm = *localtime(&l->vhost_ts);
		strftime(buf, BUFSIZE, "%b %d %T %Y %Z", &tm);
		command_success_nodata(si, "#%d Nick:\2%s\2, vhost:\2%s\2 (%s - %s)",
			x, l->nick, l->vhost, l->creator, buf);
	}
	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "WAITING");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
