/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2009 William Pitcock <nenolod -at- nenolod.net>
 * Copyright (C) 2017 Austin Ellis <siniStar@IRC4Fun.net>
 *
 * Allows requesting a vhost for a nick/account
 */

#include <atheme.h>
#include "hostserv.h"

struct hsrequest
{
	char *nick;
	char *vhost;
	time_t vhost_ts;
	char *creator;
};

bool request_per_nick;
struct service *hostsvs;

static unsigned int ratelimit_count = 0;
static time_t ratelimit_firsttime = 0;

mowgli_list_t hs_reqlist;
static char *groupmemo;

static void
write_hsreqdb(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		struct hsrequest *l = n->data;

		db_start_row(db, "HR");
		db_write_word(db, l->nick);
		db_write_word(db, l->vhost);
		db_write_time(db, l->vhost_ts);
		db_write_word(db, l->creator);
		db_commit_row(db);
	}
}

static void
db_h_hr(struct database_handle *db, const char *type)
{
	const char *nick = db_sread_word(db);
	const char *vhost = db_sread_word(db);
	time_t vhost_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);

	struct hsrequest *const l = smalloc(sizeof *l);
	l->nick = sstrdup(nick);
	l->vhost = sstrdup(vhost);
	l->vhost_ts = vhost_ts;
	l->creator = sstrdup(creator);
	mowgli_node_add(l, mowgli_node_create(), &hs_reqlist);
}

static void
nick_drop_request(struct hook_user_req *hdata)
{
	mowgli_node_t *m;
	struct hsrequest *l;

	MOWGLI_ITER_FOREACH(m, hs_reqlist.head)
	{
		l = m->data;
		if (!irccasecmp(l->nick, hdata->mn->nick))
		{
			slog(LG_REGISTER, "VHOSTREQ:DROPNICK: \2%s\2 \2%s\2", l->nick, l->vhost);

			mowgli_node_delete(m, &hs_reqlist);

			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);

			return;
		}
	}
}

static void
account_drop_request(struct myuser *mu)
{
	mowgli_node_t *n;
	struct hsrequest *l;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, entity(mu)->name))
		{
			slog(LG_REGISTER, "VHOSTREQ:DROPACCOUNT: \2%s\2 \2%s\2", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);

			return;
		}
	}
}

static void
account_delete_request(struct myuser *mu)
{
	mowgli_node_t *n;
	struct hsrequest *l;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, entity(mu)->name))
		{
			slog(LG_REGISTER, "VHOSTREQ:EXPIRE: \2%s\2 \2%s\2", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);

			return;
		}
	}
}

static void
osinfo_hook(struct sourceinfo *si)
{
	return_if_fail(si != NULL);

	// Can't think of a better way to phrase this, feel free to fix if you can.
	command_success_nodata(si, _("Requested vHosts will be per-nick: %s"), request_per_nick ? _("Yes") : _("No"));
}

static void ATHEME_FATTR_PRINTF(2, 3)
send_group_memo(struct sourceinfo *si, const char *memo, ...)
{
	struct service *msvs;
	va_list va;
	char buf[BUFSIZE];

	return_if_fail(si != NULL);
	return_if_fail(memo != NULL);

	va_start(va, memo);
	vsnprintf(buf, BUFSIZE, memo, va);
	va_end(va);

	if ((msvs = service_find("memoserv")) == NULL)
		return;
	else
	{
		char cmdbuf[BUFSIZE];

		mowgli_strlcpy(cmdbuf, groupmemo, BUFSIZE);
		mowgli_strlcat(cmdbuf, " ", BUFSIZE);
		mowgli_strlcat(cmdbuf, buf, BUFSIZE);

		command_exec_split(msvs, si, "SEND", cmdbuf, msvs->commands);
	}
}

// REQUEST <host>
static void
hs_cmd_request(struct sourceinfo *si, int parc, char *parv[])
{
	char *host = parv[0];
	const char *target;
	struct mynick *mn;
	struct myentity *mt;
	struct myuser *mu;
	struct myentity_iteration_state state;
	char buf[BUFSIZE], strfbuf[BUFSIZE];
	struct metadata *md, *md_timestamp, *md_assigner;
	mowgli_node_t *n;
	struct hsrequest *l;
	struct hook_host_request hdata;
	int matches = 0;

	if (!host)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REQUEST");
		command_fail(si, fault_needmoreparams, _("Syntax: REQUEST <vhost>"));
		return;
	}

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
	{
		ratelimit_count = 0;
		ratelimit_firsttime = CURRTIME;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from requesting vhosts by network staff."));
		return;
	}

	md = metadata_find(si->smu, "private:usercloak-timestamp");

	if (CURRTIME < (time_t)(md + config_options.vhost_change) && config_options.vhost_change > 0)
	{
		command_fail(si, fault_noprivs, _("You must wait at least \2%u\2 days between changes to your vHost."),
			(config_options.vhost_change / SECONDS_PER_DAY));
		return;
	}

	if (request_per_nick)
	{
		target = si->su != NULL ? si->su->nick : "?";
		mn = mynick_find(target);
		if (mn == NULL)
		{
			command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
			return;
		}
		if (mn->owner != si->smu)
		{
			command_fail(si, fault_noprivs, _("Nick \2%s\2 is not registered to your account."), mn->nick);
			return;
		}

		snprintf(buf, sizeof buf, "private:usercloak:%s", target);
		md = metadata_find(si->smu, buf);
		if (md != NULL && !strcmp(host, md->value))
		{
			command_success_nodata(si, _("\2%s\2 is already the active vhost for your nick."), host);
			return;
		}
	}
	else
	{
		target = entity(si->smu)->name;

		md = metadata_find(si->smu, "private:usercloak");
		if (md != NULL && !strcmp(host, md->value))
		{
			command_success_nodata(si, _("\2%s\2 is already the active vhost for your account."), host);
			return;
		}
	}

	if (!check_vhost_validity(si, host))
		return;

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		mu = user(mt);
		md = metadata_find(mu, "private:usercloak");
		if (md != NULL && !match(host, md->value))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is already assigned to another user.  You will need to request a \2different\2 vhost or see network staff."), host);
			logcommand(si, CMDLOG_REQUEST, "REQUEST:FAILED: \2%s\2 is already assigned to another user.", host);
				return;
		}
		MOWGLI_ITER_FOREACH(n, mu->nicks.head)
		{
			snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", ((struct mynick *)(n->data))->nick);
			md = metadata_find(mu, buf);
			if (md == NULL)
				continue;
			if (!match(host, md->value))
			{
			command_fail(si, fault_noprivs, _("\2%s\2 is already assigned to another user.  You will need to request a \2different\2 vhost or see network staff."), host);
			logcommand(si, CMDLOG_REQUEST, "REQUEST:FAILED: \2%s\2 is already assigned to another user.", host);
				return;
			}
		}
	}

	hdata.host = host;
	hdata.si = si;
	hdata.approved = 0;

	/* keep target and si seperate so modules that use the hook
	 * can see if it's an account or nick requesting the host
	 */
	hdata.target = target;
	hook_call_host_request(&hdata);

	if (hdata.approved != 0)
		return;

	// search for it
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
			sfree(l->vhost);
			l->vhost = sstrdup(host);
			l->vhost_ts = CURRTIME;;

			command_success_nodata(si, _("You have requested vhost \2%s\2."), host);

			if (groupmemo != NULL)
				send_group_memo(si, "[auto memo] Please review \2%s\2 for me!", host);

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

	l = smalloc(sizeof *l);
	l->nick = sstrdup(target);
	l->vhost = sstrdup(host);
	l->vhost_ts = CURRTIME;;
	l->creator = sstrdup(get_source_name(si));

	n = mowgli_node_create();
	mowgli_node_add(l, n, &hs_reqlist);

	command_success_nodata(si, _("You have requested vhost \2%s\2."), host);

        if (groupmemo != NULL)
                send_group_memo(si, "[auto memo] Please review \2%s\2 for me!", host);

	logcommand(si, CMDLOG_REQUEST, "REQUEST: \2%s\2", host);
	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;
	return;
}

// ACTIVATE <nick>
static void
hs_cmd_activate(struct sourceinfo *si, int parc, char *parv[])
{
	char *nick = parv[0];
	struct user *u;
	char buf[BUFSIZE];
	struct hsrequest *l;
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

			// VHOSTNICK command below will generate snoop
			logcommand(si, CMDLOG_REQUEST, "ACTIVATE: \2%s\2 for \2%s\2", l->vhost, nick);
			snprintf(buf, BUFSIZE, "%s %s", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);

			command_exec_split(si->service, si, request_per_nick ? "VHOSTNICK" : "VHOST", buf, si->service->commands);
			return;
		}

		if (!irccasecmp("*", nick))
		{
			if ((u = user_find_named(l->nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been approved.", l->vhost, l->nick);

			// VHOSTNICK command below will generate snoop
			logcommand(si, CMDLOG_REQUEST, "ACTIVATE: \2%s\2 for \2%s\2", l->vhost, l->nick);
			snprintf(buf, BUFSIZE, "%s %s", l->nick, l->vhost);

			mowgli_node_delete(n, &hs_reqlist);

			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);

			command_exec_split(si->service, si, request_per_nick ? "VHOSTNICK" : "VHOST", buf, si->service->commands);

			if (hs_reqlist.count == 0)
				return;
		}

	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

// REJECT <nick>
static void
hs_cmd_reject(struct sourceinfo *si, int parc, char *parv[])
{
	char *nick = parv[0];
	char *reason = parv[1];
	struct user *u;
	char buf[BUFSIZE];
	struct hsrequest *l;
	mowgli_node_t *n, *tn;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: REJECT <nick> [reason]"));
		return;
	}

	if (reason && strlen(reason) > 150)
	{
		command_fail(si, fault_badparams, _("Reason too long. It must be 150 characters or less."));
		return;
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, hs_reqlist.head)
	{
		struct service *svs;

		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((svs = service_find("memoserv")) != NULL)
			{
				if (reason)
					snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected due to: %s", nick, l->vhost, nick, reason);
				else
					snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", nick, l->vhost, nick);

				command_exec_split(svs, si, "SEND", buf, svs->commands);
			}
			else if ((u = user_find_named(nick)) != NULL)
			{
				if (reason)
					notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected due to: %s", l->vhost, nick, reason);
				else
					notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->vhost, nick);
			}

			if (reason)
				logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2, Reason: \2%s\2", l->vhost, nick, reason);
			else
				logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2", l->vhost, nick);

			mowgli_node_delete(n, &hs_reqlist);
			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);
			return;
		}

		if (!irccasecmp("*", nick))
		{
			if ((svs = service_find("memoserv")) != NULL)
			{
				if (reason)
					snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected due to: %s", nick, l->vhost, nick, reason);
				else
					snprintf(buf, BUFSIZE, "%s [auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", nick, l->vhost, nick);

				command_exec_split(svs, si, "SEND", buf, svs->commands);
			}
			else if ((u = user_find_named(l->nick)) != NULL)
			{
				if (reason)
					notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected due to: %s", l->vhost, nick, reason);
				else
					notice(si->service->nick, u->nick, "[auto memo] Your requested vhost \2%s\2 for nick \2%s\2 has been rejected.", l->vhost, nick);
			}

			if (reason)
				logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2, Reason: \2%s\2", l->vhost, l->nick, reason);
			else
				logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2 for \2%s\2", l->vhost, l->nick);

			mowgli_node_delete(n, &hs_reqlist);

			sfree(l->nick);
			sfree(l->vhost);
			sfree(l->creator);
			sfree(l);

			if (hs_reqlist.count == 0)
				return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in vhost request database."), nick);
}

// WAITING
static void
hs_cmd_waiting(struct sourceinfo *si, int parc, char *parv[])
{
	struct hsrequest *l;
	mowgli_node_t *n;
	char buf[BUFSIZE];
	struct tm *tm;

	MOWGLI_ITER_FOREACH(n, hs_reqlist.head)
	{
		l = n->data;

		tm = localtime(&l->vhost_ts);
		strftime(buf, BUFSIZE, TIME_FORMAT, tm);
		command_success_nodata(si, _("Nick: \2%s\2, vHost: \2%s\2 (%s - %s)"),
			l->nick, l->vhost, l->creator, buf);
	}
	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "WAITING");
}

static struct command hs_request = {
	.name           = "REQUEST",
	.desc           = N_("Requests new virtual hostname for current nick."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 2,
	.cmd            = &hs_cmd_request,
	.help           = { .path = "hostserv/request" },
};

static struct command hs_waiting = {
	.name           = "WAITING",
	.desc           = N_("Lists vhosts currently waiting for activation."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 1,
	.cmd            = &hs_cmd_waiting,
	.help           = { .path = "hostserv/waiting" },
};

static struct command hs_reject = {
	.name           = "REJECT",
	.desc           = N_("Reject the requested vhost for the given nick."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 2,
	.cmd            = &hs_cmd_reject,
	.help           = { .path = "hostserv/reject" },
};

static struct command hs_activate = {
	.name           = "ACTIVATE",
	.desc           = N_("Activate the requested vhost for a given nick."),
	.access         = PRIV_USER_VHOST,
	.maxparc        = 2,
	.cmd            = &hs_cmd_activate,
	.help           = { .path = "hostserv/activate" },
};

static void
mod_init(struct module *const restrict m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "hostserv/main")

	hostsvs = service_find("hostserv");

	hook_add_user_drop(account_drop_request);
	hook_add_nick_ungroup(nick_drop_request);
	hook_add_myuser_delete(account_delete_request);
	hook_add_operserv_info(osinfo_hook);
	hook_add_db_write(write_hsreqdb);

	add_dupstr_conf_item("REGGROUP", &hostsvs->conf_table, 0, &groupmemo, NULL);

	db_register_type_handler("HR", db_h_hr);

 	service_named_bind_command("hostserv", &hs_request);
	service_named_bind_command("hostserv", &hs_waiting);
	service_named_bind_command("hostserv", &hs_reject);
	service_named_bind_command("hostserv", &hs_activate);
	add_bool_conf_item("REQUEST_PER_NICK", &hostsvs->conf_table, 0, &request_per_nick, false);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("hostserv/request", MODULE_UNLOAD_CAPABILITY_NEVER)
