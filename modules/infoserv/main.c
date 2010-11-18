/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * We're basically re-implementing everything to support
 * oper info messages. Yes, I know this is a bit ugly, but 
 * I can't think of a saner way that is efficient, avoids a
 * few bugs and doesn't break people's existing InfoServ DB entries.
 *
 */

#include "atheme.h"
#include <limits.h>

DECLARE_MODULE_V1
(
	"infoserv/main", true, _modinit, _moddeinit,
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

mowgli_list_t logon_info;

struct operlogoninfo_ {
        char *nick;
        char *subject;
        time_t info_ts;
        char *story;
};

typedef struct operlogoninfo_ operlogoninfo_t;

mowgli_list_t operlogon_info;
unsigned int logoninfo_count = 0;

service_t *infoserv;
mowgli_list_t is_conftable;

static void is_cmd_help(sourceinfo_t *si, const int parc, char *parv[]);
static void is_cmd_post(sourceinfo_t *si, int parc, char *parv[]);
static void is_cmd_del(sourceinfo_t *si, int parc, char *parv[]);
static void is_cmd_odel(sourceinfo_t *si, int parc, char *parv[]);
static void is_cmd_list(sourceinfo_t *si, int parc, char *parv[]);
static void is_cmd_olist(sourceinfo_t *si, int parc, char *parv[]);
static void display_info(hook_user_nick_t *data);
static void display_oper_info(user_t *u);

static void write_infodb(database_handle_t *db);
static void db_h_li(database_handle_t *db, const char *type);

command_t is_help = { "HELP", N_(N_("Displays contextual help information.")), AC_NONE, 2, is_cmd_help, { .path = "help" } };
command_t is_post = { "POST", N_("Post news items for users to view."), PRIV_GLOBAL, 3, is_cmd_post, { .path = "infoserv/post" } };
command_t is_del = { "DEL", N_("Delete news items."), PRIV_GLOBAL, 1, is_cmd_del, { .path = "infoserv/del" } };
command_t is_odel = { "ODEL", N_("Delete oper news items."), PRIV_GLOBAL, 1, is_cmd_odel, { .path = "infoserv/odel" } };
command_t is_list = { "LIST", N_("List previously posted news items."), AC_NONE, 1, is_cmd_list, { .path = "infoserv/list" } };
/* Should prolly change the priv for this. What would be a better priv for it though? */
command_t is_olist = { "OLIST", N_("List previously posted oper news items."), PRIV_GLOBAL, 1, is_cmd_olist, { .path = "infoserv/olist" } };

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

		command_help(si, si->service->commands);

		command_success_nodata(si, _("***** \2End of Help\2 *****"));
		return;
	}

	/* take the command through the hash table */
	help_display(si, si->service, command, si->service->commands);
}

static void underscores_to_spaces(char *y)
{
	do 
	{
		if(*y == '_')
			*y = ' ';
	} while(*y++);
}

static void write_infodb(database_handle_t *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, logon_info.head)
	{
		logoninfo_t *l = n->data;

		db_start_row(db, "LI");
		db_write_word(db, l->nick);
		db_write_word(db, l->subject);
		db_write_time(db, l->info_ts);
		db_write_str(db, l->story);
		db_commit_row(db);
	}

	MOWGLI_ITER_FOREACH(n, operlogon_info.head)
	{
		operlogoninfo_t *o = n->data;

		db_start_row(db, "LIO");
		db_write_word(db, o->nick);
		db_write_word(db, o->subject);
		db_write_time(db, o->info_ts);
		db_write_str(db, o->story);
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
	mowgli_node_add(l, mowgli_node_create(), &logon_info);
}

static void db_h_lio(database_handle_t *db, const char *type)
{
	const char *nick = db_sread_word(db);
	const char *subject = db_sread_word(db);
	time_t info_ts = db_sread_time(db);
	const char *story = db_sread_str(db);

	operlogoninfo_t *o = smalloc(sizeof(operlogoninfo_t));
	o->nick = sstrdup(nick);
	o->subject = sstrdup(subject);
	o->info_ts = info_ts;
	o->story = sstrdup(story);
	mowgli_node_add(o, mowgli_node_create(), &operlogon_info);
}

static void display_info(hook_user_nick_t *data)
{
	user_t *u;
	mowgli_node_t *n;
	logoninfo_t *l;
	char dBuf[BUFSIZE];
	struct tm tm;
	unsigned int count = 0;

	u = data->u;
	if (u == NULL)
		return;

	/* abort if it's an internal client */
	if (is_internal_client(u))
		return;
	/* abort if user is coming back from split */
	if (!(u->server->flags & SF_EOB))
		return;

	if (logon_info.count > 0)
	{
		notice(infoserv->nick, u->nick, "*** \2Message(s) of the Day\2 ***");

		MOWGLI_ITER_FOREACH_PREV(n, logon_info.tail)
		{
			l = n->data;
			
			char *y = sstrdup(l->subject);
			underscores_to_spaces(y);

			tm = *localtime(&l->info_ts);
			strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", &tm);
			notice(infoserv->nick, u->nick, "[\2%s\2] Notice from %s, posted %s:",
				y, l->nick, dBuf);
			notice(infoserv->nick, u->nick, "%s", l->story);
			free(y);
			count++;

			/* only display three latest entries, max. */
			if (count == logoninfo_count)
				break;
		}

		notice(infoserv->nick, u->nick, "*** \2End of Message(s) of the Day\2 ***");
	}
}

static void display_oper_info(user_t *u)
{
	mowgli_node_t *n;
	operlogoninfo_t *o;
	char dBuf[BUFSIZE];
	struct tm tm;
	unsigned int count = 0;

	if (u == NULL)
		return;

	/* abort if it's an internal client */
	if (is_internal_client(u))
		return;
	/* abort if user is coming back from split */
	if (!(u->server->flags & SF_EOB))
		return;

	if (operlogon_info.count > 0)
	{
		notice(infoserv->nick, u->nick, "*** \2Oper Message(s) of the Day\2 ***");

		MOWGLI_ITER_FOREACH_PREV(n, operlogon_info.tail)
		{
			o = n->data;
			
			char *y = sstrdup(o->subject);
			underscores_to_spaces(y);

			tm = *localtime(&o->info_ts);
			strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", &tm);
			notice(infoserv->nick, u->nick, "[\2%s\2] Notice from %s, posted %s:",
				y, o->nick, dBuf);
			notice(infoserv->nick, u->nick, "%s", o->story);
			free(y);
			count++;

			/* only display three latest entries, max. */
			if (count == logoninfo_count)
				break;
		}

		notice(infoserv->nick, u->nick, "*** \2End of Oper Message(s) of the Day\2 ***");
	}
}

static void is_cmd_post(sourceinfo_t *si, int parc, char *parv[])
{
	char *importance = parv[0];
	char *subject = parv[1];
	char *story = parv[2];
	int imp;
	logoninfo_t *l;
	operlogoninfo_t *o;
	mowgli_node_t *n;
	char buf[BUFSIZE];

	/* make sure they're logged in */
	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!subject || !story || !importance)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "POST");
		command_fail(si, fault_needmoreparams, _("Syntax: POST <importance> <subject> <message>"));
		return;
	}

	imp = atoi(importance);

	if ((imp < 0) || (imp >=5))
	{
		command_fail(si, fault_badparams, _("Importance must be a digit between 0 and 4"));
		return;
	}
	
	char *y = sstrdup(subject);
	underscores_to_spaces(y);

	if (imp == 4)
	{
		snprintf(buf, sizeof buf, "[CRITICAL NETWORK NOTICE] %s - [%s] %s", get_source_name(si), y, story);
		msg_global_sts(infoserv->me, "*", buf);
		command_success_nodata(si, _("The InfoServ message has been sent"));
		logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, y, story);
		free(y);
		return;
	}

	if (imp == 2)
	{
		snprintf(buf, sizeof buf, "[Network Notice] %s - [%s] %s", get_source_name(si), y, story);
		notice_global_sts(infoserv->me, "*", buf);
		command_success_nodata(si, _("The InfoServ message has been sent"));
		logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, y, story);
		free(y);
		return;
	}

	if (imp == 0)
	{
		o = smalloc(sizeof(operlogoninfo_t));
		o->nick = sstrdup(entity(si->smu)->name);
		o->info_ts = CURRTIME;
		o->story = sstrdup(story);
		o->subject = sstrdup(subject);

		n = mowgli_node_create();
		mowgli_node_add(o, n, &operlogon_info);
	}

	if (imp > 0)
	{
		l = smalloc(sizeof(logoninfo_t));
		l->nick = sstrdup(entity(si->smu)->name);
		l->info_ts = CURRTIME;
		l->story = sstrdup(story);
		l->subject = sstrdup(subject);

		n = mowgli_node_create();
		mowgli_node_add(l, n, &logon_info);
	}

	command_success_nodata(si, _("Added entry to logon info"));
	logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, y, story);

	if (imp == 3)
	{
		snprintf(buf, sizeof buf, "[Network Notice] %s - [%s] %s", get_source_name(si), y, story);
		notice_global_sts(infoserv->me, "*", buf);
	}

	free(y);

	return;
}

static void is_cmd_del(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	int x = 0;
	int id;
	logoninfo_t *l;
	mowgli_node_t *n;

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
	MOWGLI_ITER_FOREACH(n, logon_info.head)
	{
		l = n->data;
		x++;

		if (x == id)
		{
			logcommand(si, CMDLOG_ADMIN, "INFO:DEL: \2%s\2, \2%s\2", l->subject, l->story);

			mowgli_node_delete(n, &logon_info);

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

static void is_cmd_odel(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	int x = 0;
	int id;
	operlogoninfo_t *o;
	mowgli_node_t *n;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ODEL");
		command_fail(si, fault_needmoreparams, "Syntax: ODEL <id>");
		return;
	}

	id = atoi(target);

	if (id <= 0)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ODEL");
		command_fail(si, fault_badparams, "Syntax: ODEL <id>");
		return;
	}

	/* search for it */
	MOWGLI_ITER_FOREACH(n, operlogon_info.head)
	{
		o = n->data;
		x++;

		if (x == id)
		{
			logcommand(si, CMDLOG_ADMIN, "INFO:ODEL: \2%s\2, \2%s\2", o->subject, o->story);

			mowgli_node_delete(n, &operlogon_info);

			free(o->nick);
			free(o->subject);
			free(o->story);
			free(o);

			command_success_nodata(si, _("Deleted entry %d from oper logon info."), id);
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("Entry %d not found in oper logon info."), id);
	return;
}

static void is_cmd_list(sourceinfo_t *si, int parc, char *parv[])
{
	logoninfo_t *l;
	mowgli_node_t *n;
	struct tm tm;
	char dBuf[BUFSIZE];
	int x = 0;

	MOWGLI_ITER_FOREACH(n, logon_info.head)
	{
		l = n->data;
		x++;

		char *y = sstrdup(l->subject);
		underscores_to_spaces(y);

		tm = *localtime(&l->info_ts);
		strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", &tm);
		command_success_nodata(si, "%d: [\2%s\2] by \2%s\2 at \2%s\2: \2%s\2", 
			x, y, l->nick, dBuf, l->story);
		free(y);
	}

	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "LIST");
	return;
}

static void is_cmd_olist(sourceinfo_t *si, int parc, char *parv[])
{
	operlogoninfo_t *o;
	mowgli_node_t *n;
	struct tm tm;
	char dBuf[BUFSIZE];
	int x = 0;

	MOWGLI_ITER_FOREACH(n, operlogon_info.head)
	{
		o = n->data;
		x++;

		char *y = sstrdup(o->subject);
		underscores_to_spaces(y);

		tm = *localtime(&o->info_ts);
		strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", &tm);
		command_success_nodata(si, "%d: [\2%s\2] by \2%s\2 at \2%s\2: \2%s\2", 
			x, y, o->nick, dBuf, o->story);
		free(y);
	}

	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "OLIST");
	return;
}

void _modinit(module_t *m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	infoserv = service_add("infoserv", NULL, &is_conftable);
	add_uint_conf_item("LOGONINFO_COUNT", &is_conftable, 0, &logoninfo_count, 0, INT_MAX, 3);

	hook_add_event("user_add");
	hook_add_user_add(display_info);
	hook_add_event("user_oper");
	hook_add_user_oper(display_oper_info);
	hook_add_db_write(write_infodb);

	db_register_type_handler("LI", db_h_li);
	db_register_type_handler("LIO", db_h_lio);

	service_bind_command(infoserv, &is_help);
	service_bind_command(infoserv, &is_post);
	service_bind_command(infoserv, &is_del);
	service_bind_command(infoserv, &is_odel);
	service_bind_command(infoserv, &is_list);
	service_bind_command(infoserv, &is_olist);
}

void _moddeinit(void)
{
	del_conf_item("LOGONINFO_COUNT", &is_conftable);

	if (infoserv)
	{
		service_delete(infoserv);
		infoserv = NULL;
	}
	
	hook_del_user_add(display_info);
	hook_del_user_oper(display_oper_info);
	hook_del_db_write(write_infodb);

	db_unregister_type_handler("LI");
	db_unregister_type_handler("LIO");

	service_unbind_command(infoserv, &is_help);
	service_unbind_command(infoserv, &is_post);
	service_unbind_command(infoserv, &is_del);
	service_unbind_command(infoserv, &is_odel);
	service_unbind_command(infoserv, &is_list);
	service_unbind_command(infoserv, &is_olist);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
