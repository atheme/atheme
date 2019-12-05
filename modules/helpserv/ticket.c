/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock <nenolod -at- nenolod.net>
 *
 * Allows requesting help for a account
 */

#include <atheme.h>

struct help_ticket
{
	stringref nick;
	time_t ticket_ts;
	char *creator;
	char *topic;
};

static unsigned int ratelimit_count = 0;
static time_t ratelimit_firsttime = 0;

static mowgli_list_t helpserv_reqlist;

static void
write_ticket_db(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
	{
		struct help_ticket *l = n->data;

		db_start_row(db, "HE");
		db_write_word(db, l->nick);
		db_write_time(db, l->ticket_ts);
		db_write_word(db, l->creator);
		db_write_str(db, l->topic);
		db_commit_row(db);
	}
}

static void
db_h_he(struct database_handle *db, const char *type)
{
	const char *nick = db_sread_word(db);
	time_t ticket_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *topic = db_sread_str(db);

	struct help_ticket *const l = smalloc(sizeof *l);
	l->nick = strshare_get(nick);
	l->ticket_ts = ticket_ts;
	l->creator = sstrdup(creator);
	l->topic = sstrdup(topic);
	mowgli_node_add(l, mowgli_node_create(), &helpserv_reqlist);
}

static void
account_drop_request(struct myuser *mu)
{
        mowgli_node_t *n;
        struct help_ticket *l;

        MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
        {
                l = n->data;
                if (!irccasecmp(l->nick, entity(mu)->name))
                {
                        slog(LG_REGISTER, "HELP:REQUEST:DROPACCOUNT: \2%s\2 \2%s\2", l->nick, l->topic);

                        mowgli_node_delete(n, &helpserv_reqlist);

                        strshare_unref(l->nick);
                        sfree(l->creator);
                        sfree(l->topic);
                        sfree(l);

                        return;
                }
        }
}

static void
account_delete_request(struct myuser *mu)
{
        mowgli_node_t *n;
        struct help_ticket *l;

        MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
        {
                l = n->data;
                if (!irccasecmp(l->nick, entity(mu)->name))
                {
                        slog(LG_REGISTER, "HELP:REQUEST:EXPIRE: \2%s\2 \2%s\2", l->nick, l->topic);

                        mowgli_node_delete(n, &helpserv_reqlist);

                        strshare_unref(l->nick);
                        sfree(l->creator);
                        sfree(l->topic);
                        sfree(l);

                        return;
                }
        }
}

// REQUEST <topic>
static void
helpserv_cmd_request(struct sourceinfo *si, int parc, char *parv[])
{
	const char *topic = parv[0];
	mowgli_node_t *n;
	struct help_ticket *l;

	if (!topic)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REQUEST");
		command_fail(si, fault_needmoreparams, _("Syntax: REQUEST <topic>"));
		return;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from requesting help by network staff."));
		return;
	}

	if ((unsigned int)(CURRTIME - ratelimit_firsttime) > config_options.ratelimit_period)
	{
		ratelimit_count = 0;
		ratelimit_firsttime = CURRTIME;
	}

	// search for it
	MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
	{
		l = n->data;

		if (!irccasecmp(l->nick, entity(si->smu)->name))
		{
			if (!strcmp(topic, l->topic))
			{
				command_success_nodata(si, _("You have already requested help about \2%s\2."), topic);
				return;
			}
			if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
			{
				command_fail(si, fault_toomany, _("The system is currently too busy to process your help request, please try again later."));
				slog(LG_INFO, "HELP:REQUEST:THROTTLED: %s", si->su->nick);
				return;
			}
			sfree(l->topic);
			l->topic = sstrdup(topic);
			l->ticket_ts = CURRTIME;;

			command_success_nodata(si, _("You have requested help about \2%s\2."), topic);
			logcommand(si, CMDLOG_REQUEST, "REQUEST: \2%s\2", topic);
			if (config_options.ratelimit_uses && config_options.ratelimit_period)
				ratelimit_count++;
			return;
		}
	}

	if (ratelimit_count > config_options.ratelimit_uses && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("The system is currently too busy to process your help request, please try again later."));
		slog(LG_INFO, "HELP:REQUEST:THROTTLED: %s", si->su->nick);
		return;
	}
	l = smalloc(sizeof *l);
	l->nick = strshare_ref(entity(si->smu)->name);
	l->ticket_ts = CURRTIME;;
	l->creator = sstrdup(get_source_name(si));
	l->topic = sstrdup(topic);

	n = mowgli_node_create();
	mowgli_node_add(l, n, &helpserv_reqlist);

	command_success_nodata(si, _("You have requested help about \2%s\2."), topic);
	logcommand(si, CMDLOG_REQUEST, "REQUEST: \2%s\2", topic);
	if (config_options.ratelimit_uses && config_options.ratelimit_period)
		ratelimit_count++;
	return;
}

// CLOSE <nick> [reason]
static void
helpserv_cmd_close(struct sourceinfo *si, int parc, char *parv[])
{
	char *nick = parv[0];
	struct user *u;
	struct help_ticket *l;
	mowgli_node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLOSE");
		command_fail(si, fault_needmoreparams, _("Syntax: CLOSE <nick> [reason]"));
		return;
	}

	MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((u = user_find_named(nick)) != NULL)
			{
				if (parv[1] != NULL)
					notice(si->service->nick, u->nick, "[auto notice] Your help request has been closed: %s", parv[1]);
				else
					notice(si->service->nick, u->nick, "[auto notice] Your help request has been closed.");
			}
			else
			{
				struct service *svs;
				char buf[BUFSIZE];

				if ((svs = service_find("memoserv")) != NULL && myuser_find(parv[0]) != NULL)
				{
					if (parv[1] != NULL)
						snprintf(buf, BUFSIZE, "%s [auto memo] Your help request has been closed: %s", parv[0], parv[1]);
					else
						snprintf(buf, BUFSIZE, "%s [auto memo] Your help request has been closed.", parv[0]);

					command_exec_split(svs, si, "SEND", buf, svs->commands);
				}
			}

			if (parv[1] != NULL)
				logcommand(si, CMDLOG_REQUEST, "CLOSE: Help for \2%s\2 about \2%s\2 (\2%s\2)", nick, l->topic, parv[1]);
			else
				logcommand(si, CMDLOG_REQUEST, "CLOSE: Help for \2%s\2 about \2%s\2", nick, l->topic);

			mowgli_node_delete(n, &helpserv_reqlist);

			strshare_unref(l->nick);
			sfree(l->creator);
			sfree(l->topic);
			sfree(l);

			return;
		}
	}

	command_success_nodata(si, _("Nick \2%s\2 not found in help request database."), nick);
}

// LIST
static void
helpserv_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	struct help_ticket *l;
	mowgli_node_t *n;
	unsigned int x = 0;
	char buf[BUFSIZE];
	struct tm *tm;

	MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
	{
		l = n->data;
		x++;

		tm = localtime(&l->ticket_ts);
		strftime(buf, BUFSIZE, TIME_FORMAT, tm);
		command_success_nodata(si, _("#%u Nick: \2%s\2, Topic: \2%s\2 (%s - %s)"),
			x, l->nick, l->topic, l->creator, buf);
	}
	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "LIST");
}

// CANCEL
static void
helpserv_cmd_cancel(struct sourceinfo *si, int parc, char *parv[])
{
        struct help_ticket *l;
        mowgli_node_t *n;

        MOWGLI_ITER_FOREACH(n, helpserv_reqlist.head)
        {
                l = n->data;

                if (!irccasecmp(l->nick, entity(si->smu)->name))
                {
                        mowgli_node_delete(n, &helpserv_reqlist);

                        strshare_unref(l->nick);
                        sfree(l->creator);
                        sfree(l->topic);
                        sfree(l);

                        command_success_nodata(si, _("Your help request has been cancelled."));

                        logcommand(si, CMDLOG_REQUEST, "CANCEL");
                        return;
                }
        }
        command_fail(si, fault_badparams, _("You do not have a help request to cancel."));
}

static struct command helpserv_request = {
	.name           = "REQUEST",
	.desc           = N_("Request help from network staff."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &helpserv_cmd_request,
	.help           = { .path = "helpserv/request" },
};

static struct command helpserv_list = {
	.name           = "LIST",
	.desc           = N_("Lists users waiting for help."),
	.access         = PRIV_HELPER,
	.maxparc        = 1,
	.cmd            = &helpserv_cmd_list,
	.help           = { .path = "helpserv/list" },
};

static struct command helpserv_close = {
	.name           = "CLOSE",
	.desc           = N_("Close a users' help request."),
	.access         = PRIV_HELPER,
	.maxparc        = 2,
	.cmd            = &helpserv_cmd_close,
	.help           = { .path = "helpserv/close" },
};

static struct command helpserv_cancel = {
	.name           = "CANCEL",
	.desc           = N_("Cancel your own pending help request."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &helpserv_cmd_cancel,
	.help           = { .path = "helpserv/cancel" },
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

	MODULE_TRY_REQUEST_DEPENDENCY(m, "helpserv/main")

	hook_add_user_drop(account_drop_request);
	hook_add_myuser_delete(account_delete_request);
	hook_add_db_write(write_ticket_db);

	db_register_type_handler("HE", db_h_he);

 	service_named_bind_command("helpserv", &helpserv_request);
	service_named_bind_command("helpserv", &helpserv_list);
	service_named_bind_command("helpserv", &helpserv_close);
	service_named_bind_command("helpserv", &helpserv_cancel);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("helpserv/ticket", MODULE_UNLOAD_CAPABILITY_NEVER)
