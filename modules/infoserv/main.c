/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the main() routine.
 *
 * We're basically re-implementing everything to support
 * oper info messages. Yes, I know this is a bit ugly, but
 * I can't think of a saner way that is efficient, avoids a
 * few bugs and doesn't break people's existing InfoServ DB entries.
 */

#include <atheme.h>

#define DB_TYPE_LOGONINFO "LI"
#define DB_TYPE_LOGONINFO_OPER "LIO"

struct logoninfo
{
	stringref nick;
	char *subject;
	time_t info_ts;
	char *story;
};

static mowgli_list_t logon_info;
static mowgli_list_t operlogon_info;

static unsigned int logoninfo_count = 0;

static struct service *infoserv = NULL;

static void
is_cmd_help(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc, char **const restrict parv)
{
	if (parv[0])
	{
		(void) help_display(si, si->service, parv[0], si->service->commands);
		return;
	}

	(void) help_display_prefix(si, si->service);
	(void) command_success_nodata(si, _("\2%s\2 allows users to view informational messages."), si->service->nick);
	(void) help_display_newline(si);
	(void) command_help(si, si->service->commands);
	(void) help_display_moreinfo(si, si->service, NULL);
	(void) help_display_suffix(si);
}

static void
underscores_to_spaces(char *y)
{
	do
	{
		if(*y == '_')
			*y = ' ';
	} while(*y++);
}

static void
write_infoentry(struct database_handle *db, struct logoninfo *l, bool oper)
{
	db_start_row(db, oper ? DB_TYPE_LOGONINFO_OPER : DB_TYPE_LOGONINFO);
	db_write_word(db, l->nick);
	db_write_word(db, l->subject);
	db_write_time(db, l->info_ts);
	db_write_str(db, l->story);
	db_commit_row(db);
}

static void
write_infodb(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, logon_info.head)
	{
		write_infoentry(db, n->data, false);
	}

	MOWGLI_ITER_FOREACH(n, operlogon_info.head)
	{
		write_infoentry(db, n->data, true);
	}

}

static void
db_h_li(struct database_handle *db, const char *type)
{
	const char *nick = db_sread_word(db);
	const char *subject = db_sread_word(db);
	time_t info_ts = db_sread_time(db);
	const char *story = db_sread_str(db);

	struct logoninfo *const l = smalloc(sizeof *l);
	l->nick = strshare_get(nick);
	l->subject = sstrdup(subject);
	l->info_ts = info_ts;
	l->story = sstrdup(story);

	if (strcasecmp(type, DB_TYPE_LOGONINFO_OPER) == 0)
		mowgli_node_add(l, mowgli_node_create(), &operlogon_info);
	else
		mowgli_node_add(l, mowgli_node_create(), &logon_info);
}

static void
display_info(struct user *u, bool oper)
{
	return_if_fail(u != NULL);

	// abort if it's an internal client
	if (is_internal_client(u))
		return;
	// abort if user is coming back from split
	if (!(u->server->flags & SF_EOB))
		return;

	mowgli_list_t *list = (oper ? &operlogon_info : &logon_info);
	if (list->count > 0)
	{
		if (oper)
			notice(infoserv->nick, u->nick, "*** \2Oper Message(s) of the Day\2 ***");
		else
			notice(infoserv->nick, u->nick, "*** \2Message(s) of the Day\2 ***");

		mowgli_node_t *n;
		unsigned int count = 0;
		MOWGLI_ITER_FOREACH_PREV(n, list->tail)
		{
			struct logoninfo *l = n->data;

			char *y = sstrdup(l->subject);
			underscores_to_spaces(y);

			struct tm *tm;
			tm = localtime(&l->info_ts);

			char dBuf[BUFSIZE];
			strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", tm);

			notice(infoserv->nick, u->nick, "[\2%s\2] Notice from %s, posted %s:",
				y, l->nick, dBuf);
			notice(infoserv->nick, u->nick, "%s", l->story);
			sfree(y);
			count++;

			// only display three latest entries, max.
			if (count == logoninfo_count)
				break;
		}

		if (oper)
			notice(infoserv->nick, u->nick, "*** \2End of Oper Message(s) of the Day\2 ***");
		else
			notice(infoserv->nick, u->nick, "*** \2End of Message(s) of the Day\2 ***");
	}
}

static void
hook_user_add(struct hook_user_nick *hdata)
{
	if (hdata->u)
		display_info(hdata->u, false);
}

static void
hook_user_oper(struct user *u)
{
	display_info(u, true);
}

static void
osinfo_hook(struct sourceinfo *si)
{
	return_if_fail(si != NULL);

	command_success_nodata(si, _("How many messages will be sent to users on connect: %u"), logoninfo_count);
}

static void
is_cmd_post(struct sourceinfo *si, int parc, char *parv[])
{
	char *importance = parv[0];
	char *subject = parv[1];
	char *story = parv[2];

	// make sure they're logged in
	if (!si->smu)
	{
		command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
		return;
	}

	if (!subject || !story || !importance)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "POST");
		command_fail(si, fault_needmoreparams, _("Syntax: POST <importance> <subject> <message>"));
		return;
	}

	unsigned int imp;
	if (! string_to_uint(importance, &imp) || imp > 4)
	{
		command_fail(si, fault_badparams, _("Importance must be a digit between 0 and 4"));
		return;
	}

	char *y = sstrdup(subject);
	underscores_to_spaces(y);

	if (imp == 4)
	{
		char buf[BUFSIZE];
		snprintf(buf, sizeof buf, "[CRITICAL NETWORK NOTICE] %s - [%s] %s", get_source_name(si), y, story);
		msg_global_sts(infoserv->me, "*", buf);
		command_success_nodata(si, _("The InfoServ message has been sent"));
		logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, y, story);
		sfree(y);
		return;
	}

	if (imp == 2)
	{
		char buf[BUFSIZE];
		snprintf(buf, sizeof buf, "[Network Notice] %s - [%s] %s", get_source_name(si), y, story);
		notice_global_sts(infoserv->me, "*", buf);
		command_success_nodata(si, _("The InfoServ message has been sent"));
		logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, y, story);
		sfree(y);
		return;
	}

	struct logoninfo *l = smalloc(sizeof *l);
	l->nick = strshare_ref(entity(si->smu)->name);
	l->info_ts = CURRTIME;
	l->story = sstrdup(story);
	l->subject = sstrdup(subject);

	mowgli_node_t *n = mowgli_node_create();
	mowgli_node_add(l, n, (imp == 0) ? &operlogon_info : &logon_info);

	command_success_nodata(si, _("Added entry to logon info"));
	logcommand(si, CMDLOG_ADMIN, "INFO:POST: Importance: \2%s\2, Subject: \2%s\2, Message: \2%s\2", importance, y, story);

	if (imp == 3)
	{
		char buf[BUFSIZE];
		snprintf(buf, sizeof buf, "[Network Notice] %s - [%s] %s", get_source_name(si), y, story);
		notice_global_sts(infoserv->me, "*", buf);
	}

	sfree(y);

	return;
}

static void
is_del_impl(struct sourceinfo *si, int parc, char *parv[], bool oper)
{
	char *target = parv[0];

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, oper ? "ODEL" : "DEL");
		if (oper)
			command_fail(si, fault_needmoreparams, _("Syntax: ODEL <id>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: DEL <id>"));
		return;
	}

	unsigned int id;
	if (! string_to_uint(target, &id) || ! id)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, oper ? "ODEL" : "DEL");
		if (oper)
			command_fail(si, fault_needmoreparams, _("Syntax: ODEL <id>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: DEL <id>"));
		return;
	}

	mowgli_list_t *list = (oper ? &operlogon_info : &logon_info);

	unsigned int x = 0;
	struct logoninfo *l;
	mowgli_node_t *n;

	// search for it
	MOWGLI_ITER_FOREACH(n, list->head)
	{
		l = n->data;
		x++;

		if (x == id)
		{
			logcommand(si, CMDLOG_ADMIN, "INFO:%s: \2%s\2, \2%s\2", oper ? "ODEL" : "DEL", l->subject, l->story);

			mowgli_node_delete(n, list);
			mowgli_node_free(n);

			strshare_unref(l->nick);
			sfree(l->subject);
			sfree(l->story);
			sfree(l);

			if (oper)
				command_success_nodata(si, _("Deleted entry %u from oper logon info."), id);
			else
				command_success_nodata(si, _("Deleted entry %u from logon info."), id);
			return;
		}
	}

	if (oper)
		command_fail(si, fault_nosuch_target, _("Entry \2%u\2 not found in oper logon info."), id);
	else
		command_fail(si, fault_nosuch_target, _("Entry \2%u\2 not found in logon info."), id);
	return;
}

static void
is_cmd_del(struct sourceinfo *si, int parc, char *parv[])
{
	is_del_impl(si, parc, parv, false);
}

static void
is_cmd_odel(struct sourceinfo *si, int parc, char *parv[])
{
	is_del_impl(si, parc, parv, true);
}

static void
is_list_impl(struct sourceinfo *si, int parc, char *parv[], bool oper)
{
	mowgli_node_t *n;
	unsigned int x = 0;

	MOWGLI_ITER_FOREACH(n, (oper ? operlogon_info : logon_info).head)
	{
		struct logoninfo *l = n->data;
		x++;

		char *y = sstrdup(l->subject);
		underscores_to_spaces(y);

		struct tm *tm;
		tm = localtime(&l->info_ts);

		char dBuf[BUFSIZE];
		strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", tm);

		command_success_nodata(si, _("%u: [\2%s\2] by \2%s\2 at \2%s\2: \2%s\2"),
			x, y, l->nick, dBuf, l->story);

		sfree(y);
	}

	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, (oper ? "OLIST" : "LIST"));
	return;
}

static void
is_cmd_list(struct sourceinfo *si, int parc, char *parv[])
{
	is_list_impl(si, parc, parv, false);
}

static void
is_cmd_olist(struct sourceinfo *si, int parc, char *parv[])
{
	is_list_impl(si, parc, parv, true);
}

static struct command is_help = {
	.name           = "HELP",
	.desc           = STR_HELP_DESCRIPTION,
	.access         = AC_NONE,
	.maxparc        = 2,
	.cmd            = &is_cmd_help,
	.help           = { .path = "help" },
};

static struct command is_post = {
	.name           = "POST",
	.desc           = N_("Post news items for users to view."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 3,
	.cmd            = &is_cmd_post,
	.help           = { .path = "infoserv/post" },
};

static struct command is_del = {
	.name           = "DEL",
	.desc           = N_("Delete news items."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 1,
	.cmd            = &is_cmd_del,
	.help           = { .path = "infoserv/del" },
};

static struct command is_odel = {
	.name           = "ODEL",
	.desc           = N_("Delete oper news items."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 1,
	.cmd            = &is_cmd_odel,
	.help           = { .path = "infoserv/odel" },
};

static struct command is_list = {
	.name           = "LIST",
	.desc           = N_("List previously posted news items."),
	.access         = AC_NONE,
	.maxparc        = 1,
	.cmd            = &is_cmd_list,
	.help           = { .path = "infoserv/list" },
};

// Should probably change the priv for this. What would be a better priv for it though?
static struct command is_olist = {
	.name           = "OLIST",
	.desc           = N_("List previously posted oper news items."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 1,
	.cmd            = &is_cmd_olist,
	.help           = { .path = "infoserv/olist" },
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

	infoserv = service_add("infoserv", NULL);
	add_uint_conf_item("LOGONINFO_COUNT", &infoserv->conf_table, 0, &logoninfo_count, 0, INT_MAX, 3);

	hook_add_user_add(hook_user_add);
	hook_add_user_oper(hook_user_oper);
	hook_add_operserv_info(osinfo_hook);
	hook_add_db_write(write_infodb);

	db_register_type_handler(DB_TYPE_LOGONINFO, db_h_li);
	db_register_type_handler(DB_TYPE_LOGONINFO_OPER, db_h_li);

	service_bind_command(infoserv, &is_help);
	service_bind_command(infoserv, &is_post);
	service_bind_command(infoserv, &is_del);
	service_bind_command(infoserv, &is_odel);
	service_bind_command(infoserv, &is_list);
	service_bind_command(infoserv, &is_olist);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("infoserv/main", MODULE_UNLOAD_CAPABILITY_NEVER)
