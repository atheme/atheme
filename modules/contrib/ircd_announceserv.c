/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/ircd_announceserv", false, _modinit, _moddeinit,
	PACKAGE_STRING, 
	"JD and Taros"
);

service_t *announcesvs;

static void as_cmd_help(sourceinfo_t *si, int parc, char *parv[]);
static void account_drop_request(myuser_t *mu);
static void as_cmd_request(sourceinfo_t *si, int parc, char *parv[]);
static void as_cmd_waiting(sourceinfo_t *si, int parc, char *parv[]);
static void as_cmd_reject(sourceinfo_t *si, int parc, char *parv[]);
static void as_cmd_activate(sourceinfo_t *si, int parc, char *parv[]);
static void as_cmd_cancel(sourceinfo_t *si, int parc, char *parv[]);
static void write_asreqdb(database_handle_t *db);
static void db_h_ar(database_handle_t *db, const char *type);

command_t as_help = { "HELP", N_(N_("Displays contextual help information.")), AC_NONE, 2, as_cmd_help, { .path = "help/help" } };
command_t as_request = { "REQUEST", N_("Requests new announcement."), AC_AUTHENTICATED, 2, as_cmd_request, { .path = "contrib/as_request" } };
command_t as_waiting = { "WAITING", N_("Lists announcements currently waiting for activation."), PRIV_GLOBAL, 1, as_cmd_waiting, { .path = "contrib/as_waiting" } };
command_t as_reject = { "REJECT", N_("Reject the requested announcement for the given nick."), PRIV_GLOBAL, 2, as_cmd_reject, { .path = "contrib/as_reject" } };
command_t as_activate = { "ACTIVATE", N_("Activate the requested announcement for a given nick."), PRIV_GLOBAL, 2, as_cmd_activate, { .path = "contrib/as_activate" } };
command_t as_cancel = { "CANCEL", N_("Cancels your requested announcement."), AC_AUTHENTICATED, 0, as_cmd_cancel, { .path = "contrib/as_cancel" } };

struct asreq_ {
	char *nick;
	char *subject;
	time_t announce_ts;
	char *creator;
	char *text;
};

typedef struct asreq_ asreq_t;

mowgli_list_t as_reqlist;

void _modinit(module_t *m)
{
	announcesvs = service_add("announceserv", NULL);
	
	hook_add_event("user_drop");
	hook_add_user_drop(account_drop_request);
	
	hook_add_db_write(write_asreqdb);
	db_register_type_handler("AR", db_h_ar);

	service_named_bind_command("announceserv", &as_help);
	service_named_bind_command("announceserv", &as_request);
	service_named_bind_command("announceserv", &as_waiting);
	service_named_bind_command("announceserv", &as_reject);
	service_named_bind_command("announceserv", &as_activate);
	service_named_bind_command("announceserv", &as_cancel);
}

void _moddeinit(module_unload_intent_t intent)
{
	if (announcesvs)
	{
		service_delete(announcesvs);
		announcesvs = NULL;
	}
	
	hook_del_user_drop(account_drop_request);
	hook_del_db_write(write_asreqdb);
	db_unregister_type_handler("AR");

	service_named_unbind_command("announceserv", &as_help);
	service_named_unbind_command("announceserv", &as_request);
	service_named_unbind_command("announceserv", &as_waiting);
	service_named_unbind_command("announceserv", &as_reject);
	service_named_unbind_command("announceserv", &as_activate);
	service_named_unbind_command("announceserv", &as_cancel);
}

static void write_asreqdb(database_handle_t *db)
{
	mowgli_node_t *n;

	MOWGLI_LIST_FOREACH(n, as_reqlist.head)
	{
		asreq_t *l = n->data;

		db_start_row(db, "AR");
		db_write_word(db, l->nick);
		db_write_word(db, l->subject);
		db_write_time(db, l->announce_ts);
		db_write_word(db, l->creator);
		db_write_str(db, l->text);
		db_commit_row(db);
	}

}

static void db_h_ar(database_handle_t *db, const char *type)
{
	const char *nick = db_sread_word(db);
	const char *subject = db_sread_word(db);
	time_t announce_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *text = db_sread_str(db);

	asreq_t *l = smalloc(sizeof(asreq_t));
	l->nick = strshare_get(nick);
	l->creator = strshare_get(creator);
	l->subject = sstrdup(subject);
	l->announce_ts = announce_ts;
	l->text = sstrdup(text);
	mowgli_node_add(l, mowgli_node_create(), &as_reqlist);
}

/* Properly remove announcement requests from the DB if an account is dropped */
static void account_drop_request(myuser_t *mu)
{
	mowgli_node_t *n;
	asreq_t *l;

	MOWGLI_LIST_FOREACH(n, as_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, entity(mu)->name))
		{
			slog(LG_REGISTER, "ANNOUNCEREQ:DROPACCOUNT: \2%s\2 %s\2", l->nick, l->text);

			mowgli_node_delete(n, &as_reqlist);

			strshare_unref(l->nick);
			strshare_unref(l->creator);
			free(l->subject);
			free(l->text);
			free(l);

			return;
		}
	}
}

/* HELP <command> [params] */
void as_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 allows users to request a network announcement."), si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		command_success_nodata(si, " ");

		command_help(si, si->service->commands);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, si->service->commands);
}

static void as_cmd_request(sourceinfo_t *si, int parc, char *parv[])
{
	char *subject = parv[0];
	char *text = parv[1];
	char *target;
	char *subject2;
	char buf [BUFSIZE];
	mowgli_node_t *n;
	asreq_t *l;

	if (!text || !subject)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REQUEST");
		command_fail(si, fault_needmoreparams, _("Syntax: REQUEST <subject> <text>"));
		return;
	}

	if (metadata_find(si->smu, "private:restrict:setter"))
	{
		command_fail(si, fault_noprivs, _("You have been restricted from requesting announcements by network staff."));
		return;
	}

	target = entity(si->smu)->name;

	MOWGLI_LIST_FOREACH(n, as_reqlist.head)
	{
		l = n->data;

		if (!irccasecmp(l->nick, target))
		{
			command_fail(si, fault_badparams, _("You cannot request more than one announcement. Use CANCEL if you wish to cancel your current announcement and submit another."));
			return;
		}
	}

	/* Check the subject for being too long as well. 35 chars is probably a safe limit here.
	 * Used here because we don't want users that don't know any better making too-long messages.
	 */
	if (strlen(subject) > 35)
	{
		command_fail(si, fault_badparams, _("Your subject is too long. Subjects need to be under 35 characters."));
		return;
	}

	/* Check if the announcement is too long or not. 450 characters is safe for our usecase */
	if (strlen(text) > 450)
	{
		command_fail(si, fault_badparams, _("Your announcement is too long. Announcements need to be under 450 characters."));
		return;
	}

	snprintf(buf, BUFSIZE, "%s", text);

	l = smalloc(sizeof(asreq_t));
	l->nick = strshare_ref(target);
	l->subject = sstrdup(subject);
	l->announce_ts = CURRTIME;;
	l->creator = strshare_ref(target);
	l->text = sstrdup(buf);

	n = mowgli_node_create();
	mowgli_node_add(l, n, &as_reqlist);

	subject2 = sstrdup(l->subject);
	/* This doesn't need to be as efficient as InfoServ, so let's just use replace() */
	replace(subject2, BUFSIZE, "_", " ");

	command_success_nodata(si, _("You have requested the following announcement: "));
	command_success_nodata(si, _("[%s - %s] %s"), subject2, l->creator, buf);
	/* This is kind of hacky, and the slog will come from operserv, not announceserv.
	 * Still, it's required so the message will cut off properly, and I can't find a less hacky way to do it. */
	logcommand(si, CMDLOG_REQUEST, "REQUEST:");
	slog(CMDLOG_REQUEST, "[%s - %s] %s", subject2, l->creator, buf);
	free(subject2);

	return;
}

static void as_cmd_activate(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick = parv[0];
	user_t *u;
	char *subject2;
	char buf[BUFSIZE];
	asreq_t *l;
	mowgli_node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACTIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: ACTIVATE <nick>"));
		return;
	}


	MOWGLI_LIST_FOREACH(n, as_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((u = user_find_named(nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested announcement has been approved.");
			subject2 = sstrdup(l->subject);
			replace(subject2, BUFSIZE, "_", " ");
			logcommand(si, CMDLOG_REQUEST, "ACTIVATE: \2%s\2", nick);
			snprintf(buf, BUFSIZE, "[%s - %s] %s", subject2, l->creator, l->text);

			mowgli_node_delete(n, &as_reqlist);

			free(subject2);
			free(l->nick);
			free(l->subject);
			free(l->creator);
			free(l->text);
			free(l);

			notice_global_sts(si->service->me, "*", buf);
			return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in announce request database."), nick);
}

static void as_cmd_reject(sourceinfo_t *si, int parc, char *parv[])
{
	char *nick = parv[0];
	user_t *u;
	asreq_t *l;
	mowgli_node_t *n;

	if (!nick)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: REJECT <nick>"));
		return;
	}


	MOWGLI_LIST_FOREACH(n, as_reqlist.head)
	{
		l = n->data;
		if (!irccasecmp(l->nick, nick))
		{
			if ((u = user_find_named(nick)) != NULL)
				notice(si->service->nick, u->nick, "[auto memo] Your requested announcement has been rejected.");
			logcommand(si, CMDLOG_REQUEST, "REJECT: \2%s\2", nick);

			mowgli_node_delete(n, &as_reqlist);
			free(l->nick);
			free(l->subject);
			free(l->creator);
			free(l->text);
			free(l);
			return;
		}
	}
	command_success_nodata(si, _("Nick \2%s\2 not found in announcement request database."), nick);
}

static void as_cmd_waiting(sourceinfo_t *si, int parc, char *parv[])
{
	asreq_t *l;
	mowgli_node_t *n;
	char *subject2;
	char buf[BUFSIZE];
	struct tm tm;

	MOWGLI_LIST_FOREACH(n, as_reqlist.head)
	{
		l = n->data;

		tm = *localtime(&l->announce_ts);
		strftime(buf, BUFSIZE, config_options.time_format, &tm);
		subject2 = sstrdup(l->subject);
		replace(subject2, BUFSIZE, "_", " ");
		/* This needs to be two lines for cutoff purposes */
		command_success_nodata(si, "Account:\2%s\2, Subject: %s, Requested On: \2%s\2, Announcement:",
			l->nick, subject2, buf);
		command_success_nodata(si, "%s", l->text);
		free(subject2);
	}
	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "WAITING");
}

static void as_cmd_cancel(sourceinfo_t *si, int parc, char *parv[])
{
	asreq_t *l;
	mowgli_node_t *n;
	char *target;

	target = entity(si->smu)->name;

        MOWGLI_LIST_FOREACH(n, as_reqlist.head)
        {
                l = n->data;

                if (!irccasecmp(l->nick, target))
		{
			mowgli_node_delete(n, &as_reqlist);

			strshare_unref(l->nick);
			strshare_unref(l->creator);
			free(l->subject);
			free(l->text);
			free(l);

			command_success_nodata(si, "Your pending announcement has been canceled.");

			logcommand(si, CMDLOG_REQUEST, "CANCEL");
			return;
                }
        }
	command_fail(si, fault_badparams, _("You do not have a pending announcement to cancel."));
}
/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
