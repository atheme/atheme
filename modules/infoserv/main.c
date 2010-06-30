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
	"infoserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

struct logoninfo_ {
        char *nick;
        char *subject;
        time_t info_ts;
        char *story;
};

typedef struct logoninfo_ logoninfo_t;

list_t logon_info;

service_t *infoserv;
list_t is_cmdtree;
list_t is_helptree;
list_t is_conftable;

static void is_cmd_help(sourceinfo_t *si, const int parc, char *parv[]);
static void is_cmd_post(sourceinfo_t *si, int parc, char *parv[]);
static void is_cmd_del(sourceinfo_t *si, int parc, char *parv[]);
static void is_cmd_list(sourceinfo_t *si, int parc, char *parv[]);
static void display_info(hook_user_nick_t *data);

static void write_infodb(database_handle_t *db);
static void db_h_li(database_handle_t *db, const char *type);

command_t is_help = { "HELP", N_(N_("Displays contextual help information.")), AC_NONE, 2, is_cmd_help };
command_t is_post = { "POST", N_("Post news items for users to view."), PRIV_GLOBAL, 3, is_cmd_post };
command_t is_del = { "DEL", N_("Delete news items."), PRIV_GLOBAL, 1, is_cmd_del };
command_t is_list = { "LIST", N_("List previously posted news items."), AC_NONE, 1, is_cmd_list };

/* HELP <command> [params] */
void is_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), si->service->nick);
		command_success_nodata(si, _("\2%s\2 allows users to view informational messages."), si->service->nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == false) ? "msg " : "", infoserv->disp);
		command_success_nodata(si, " ");

		command_help(si, &is_cmdtree);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, &is_helptree);
}

static void write_infodb(database_handle_t *db)
{
	node_t *n;

	LIST_FOREACH(n, logon_info.head)
	{
		logoninfo_t *l = n->data;

		db_start_row(db, "LI");
		db_write_word(db, l->nick);
		db_write_word(db, l->subject);
		db_write_time(db, l->info_ts);
		db_write_str(db, l->story);
		db_commit_row(db);
	}

}

static void db_h_li(database_handle_t *db, const char *type)
{
	const char *nick = db_sread_word(db);
	const char *subject = db_sread_word(db);
	time_t info_ts = db_sread_time(db);
	const char *story = db_sread_str(db);

	logoninfo_t *l = smalloc(sizeof(logoninfo_t));
	l->nick = sstrdup(nick);
	l->subject = sstrdup(subject);
	l->info_ts = info_ts;
	l->story = sstrdup(story);
	node_add(l, node_create(), &logon_info);
}

static void display_info(hook_user_nick_t *data)
{
	user_t *u;
	node_t *n;
	logoninfo_t *l;
	char dBuf[BUFSIZE];
	struct tm tm;
	int count = 0;

	u = data->u;
	if (u == NULL)
		return;

	/* abort if it's an internal client */
	if (is_internal_client(u))
		return;

	if (logon_info.count > 0)
	{
		notice(infoserv->nick, u->nick, "*** \2Message(s) of the Day\2 ***");

		LIST_FOREACH_PREV(n, logon_info.tail)
		{
			l = n->data;
			tm = *localtime(&l->info_ts);
			strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", &tm);
			notice(infoserv->nick, u->nick, "[\2%s\2] Notice from %s, posted %s:",
				l->subject, l->nick, dBuf);
			notice(infoserv->nick, u->nick, "%s", l->story);
			count++;

			/* only display three latest entries, max. */
			if (count == 3)
				break;
		}

		notice(infoserv->nick, u->nick, "*** \2End of Message(s) of the Day\2 ***");
	}
}

static void is_cmd_post(sourceinfo_t *si, int parc, char *parv[])
{
	char *importance = parv[0];
	char *subject = parv[1];
	char *story = parv[2];
	int imp;
	logoninfo_t *l;
	node_t *n;
	char buf[BUFSIZE];

	if (!subject || !story || !importance)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "POST");
		command_fail(si, fault_needmoreparams, _("Syntax: POST <importance> <subject> <message>"));
		return;
	}

	imp = atoi(importance);

	if ((imp <= 0) || (imp >=5))
	{
		command_fail(si, fault_badparams, _("Importance must be a digit between 1 and 4"));
		return;
	}

	if (imp == 4)
	{
		snprintf(buf, sizeof buf, "[CRITICAL NETWORK NOTICE] %s - [%s] %s", get_source_name(si), subject, story);
		msg_global_sts(infoserv->me, "*", buf);
		command_success_nodata(si, _("The InfoServ message has been sent"));
		logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, subject, story);
		return;
	}

	if (imp == 2)
	{
		snprintf(buf, sizeof buf, "[Network Notice] %s - [%s] %s", get_source_name(si), subject, story);
		notice_global_sts(infoserv->me, "*", buf);
		command_success_nodata(si, _("The InfoServ message has been sent"));
		logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, subject, story);
		return;
	}

	l = smalloc(sizeof(logoninfo_t));
	l->nick = sstrdup(get_source_name(si));
	l->info_ts = CURRTIME;
	l->story = sstrdup(story);
	l->subject = sstrdup(subject);

	n = node_create();
	node_add(l, n, &logon_info);

	command_success_nodata(si, _("Added entry to logon info"));
	logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, subject, story);

	if (imp == 3)
	{
		snprintf(buf, sizeof buf, "[Network Notice] %s - [%s] %s", get_source_name(si), subject, story);
		notice_global_sts(infoserv->me, "*", buf);
	}

	return;
}

static void is_cmd_del(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	int x = 0;
	int id;
	logoninfo_t *l;
	node_t *n;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEL");
		command_fail(si, fault_needmoreparams, "Syntax: DEL <id>");
		return;
	}

	id = atoi(target);

	if (id <= 0)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "DEL");
		command_fail(si, fault_badparams, "Syntax: DEL <id>");
		return;
	}

	/* search for it */
	LIST_FOREACH(n, logon_info.head)
	{
		l = n->data;
		x++;

		if (x == id)
		{
			logcommand(si, CMDLOG_ADMIN, "INFO:DEL: \2%s\2, \2%s\2", l->subject, l->story);

			node_del(n, &logon_info);

			free(l->nick);
			free(l->subject);
			free(l->story);
			free(l);

			command_success_nodata(si, _("Deleted entry %d from logon info."), id);
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("Entry %d not found in logon info."), id);
	return;
}

static void is_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	logoninfo_t *l;
	node_t *n;
	struct tm tm;
	char dBuf[BUFSIZE];
	int x = 0;

	LIST_FOREACH(n, logon_info.head)
	{
		l = n->data;
		x++;


		tm = *localtime(&l->info_ts);
		strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", &tm);
		command_success_nodata(si, "%d: [\2%s\2] by \2%s\2 at \2%s\2: \2%s\2", 
			x, l->subject, l->nick, dBuf, l->story);
	}

	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "LIST");
	return;
}

/* main services client routine */
static void infoserv_handler(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
        char *text;
	char orig[BUFSIZE];

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

	/* take the command through the hash table */
	command_exec_split(si->service, si, cmd, text, &is_cmdtree);
}

void _modinit(module_t *m)
{
	infoserv = service_add("infoserv", infoserv_handler, &is_cmdtree, &is_conftable);

	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	hook_add_event("user_add");
	hook_add_user_add(display_info);
	hook_add_db_write(write_infodb);

	db_register_type_handler("LI", db_h_li);

	command_add(&is_help, &is_cmdtree);
	command_add(&is_post, &is_cmdtree);
	command_add(&is_del, &is_cmdtree);
	command_add(&is_list, &is_cmdtree);

	help_addentry(&is_helptree, "HELP", "help/help", NULL);
	help_addentry(&is_helptree, "POST", "help/infoserv/post", NULL);
	help_addentry(&is_helptree, "DEL", "help/infoserv/del", NULL);
	help_addentry(&is_helptree, "LIST", "help/infoserv/list", NULL);
}

void _moddeinit(void)
{
	if (infoserv)
	{
		service_delete(infoserv);
		infoserv = NULL;
	}
	
	hook_del_user_add(display_info);
	hook_del_db_write(write_infodb);

	db_unregister_type_handler("LI");

	command_delete(&is_help, &is_cmdtree);
	command_delete(&is_post, &is_cmdtree);
	command_delete(&is_del, &is_cmdtree);
	command_delete(&is_list, &is_cmdtree);

	help_delentry(&is_helptree, "HELP");
	help_delentry(&is_helptree, "POST");
	help_delentry(&is_helptree, "DEL");
	help_delentry(&is_helptree, "LIST");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
