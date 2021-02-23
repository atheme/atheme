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

#define INFOSERV_PERSIST_STORAGE_NAME "atheme.infoserv.main.persist"
#define INFOSERV_PERSIST_VERSION      1U

struct logoninfo
{
	stringref nick;
	char *subject;
	time_t info_ts;
	char *story;
	mowgli_node_t node;
};

struct infoserv_persist_record
{
	unsigned int version;
	mowgli_list_t logon_info, operlogon_info;
};

static mowgli_list_t logon_info;
static mowgli_list_t operlogon_info;

static unsigned int logoninfo_count = 0;
static bool logoninfo_reverse = true;
static bool logoninfo_show_metadata = true;

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
		mowgli_node_add(l, &l->node, &operlogon_info);
	else
		mowgli_node_add(l, &l->node, &logon_info);
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

		mowgli_node_t *n = (logoninfo_reverse ? list->tail : list->head);
		unsigned int count = 0;

		// can't use MOWGLI_ITER_FOREACH as we need to be able to adjust the direction
		while (n)
		{
			struct logoninfo *l = n->data;

			char *y = sstrdup(l->subject);
			underscores_to_spaces(y);

			if (logoninfo_show_metadata)
			{
				struct tm *tm;
				tm = localtime(&l->info_ts);

				char dBuf[BUFSIZE];
				strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", tm);

				notice(infoserv->nick, u->nick, "[\2%s\2] Notice from %s, posted %s:",
						y, l->nick, dBuf);
				notice(infoserv->nick, u->nick, "%s", l->story);
			}
			else
			{
				notice(infoserv->nick, u->nick, "[\2%s\2] %s", y, l->story);
			}

			sfree(y);
			count++;

			// only display up to the configured number of entries
			if (logoninfo_count != 0 && count == logoninfo_count)
				break;

			n = (logoninfo_reverse ? n->prev : n->next);
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

	mowgli_list_t *list = (imp == 0) ? &operlogon_info : &logon_info;
	mowgli_node_add(l, &l->node, list);

	command_success_nodata(si, _("Added entry to logon info"));
	if (logoninfo_count != 0 && !logoninfo_reverse && list->count > logoninfo_count)
		command_success_nodata(si, ngettext("Warning: There is more than %u entry, so your message will not be displayed by default.",
		                                    "Warning: There are more than %u entries, so your message will not be displayed by default.",
		                                    logoninfo_count), logoninfo_count);

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
			command_fail(si, fault_badparams, _("Syntax: ODEL <id>"));
		else
			command_fail(si, fault_badparams, _("Syntax: DEL <id>"));
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
is_move_impl(struct sourceinfo *si, int parc, char *parv[], bool oper)
{
	char *from = parv[0];
	char *to   = parv[1];

	if (!to)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, (oper ? "OMOVE" : "MOVE"));
		if (oper)
			command_fail(si, fault_needmoreparams, _("Syntax: OMOVE <from> <to>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: MOVE <from> <to>"));
		return;
	}

	unsigned int from_id, to_id;
	if (! string_to_uint(from, &from_id) || ! string_to_uint(to, &to_id))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, (oper ? "OMOVE" : "MOVE"));
		if (oper)
			command_fail(si, fault_badparams, _("Syntax: OMOVE <from> <to>"));
		else
			command_fail(si, fault_badparams, _("Syntax: MOVE <from> <to>"));
		return;
	}

	mowgli_list_t *list = (oper ? &operlogon_info : &logon_info);

	if (!from_id || from_id > list->count)
	{
		command_fail(si, fault_nosuch_target, _("The specified entry does not exist."));
		return;
	}

	if (!to_id || to_id > list->count)
	{
		command_fail(si, fault_nosuch_target, _("The target position does not exist."));
		return;
	}

	if (from_id == to_id)
	{
		command_fail(si, fault_nochange, _("You must specify two different positions."));
		return;
	}

	mowgli_node_t *from_node = NULL, *to_node = NULL, *n;
	struct logoninfo *entry = NULL;

	unsigned int pos = 0;
	MOWGLI_ITER_FOREACH(n, list->head)
	{
		// 1-based indexing, reflecting what we display in LIST output
		pos++;

		if (!from_node && pos == from_id)
		{
			/* We'll be removing this node from the list and adding it before the
			 * item currently at the target position. We need to skip this node
			 * in the count in case the target position is after it, however, as
			 * we're going to be removing it.
			 */
			pos--;
			from_node = n;
			entry = n->data;
		}

		if (!to_node && pos == to_id)
			to_node = n;

		if (from_node && to_node)
			break;
	}

	/* this should never happen
	 * note that to_node can be NULL if we're asked to move an item to the end of the list;
	 * mowgli_node_insert_before will do the right thing in that case
	 */
	if (!from_node || from_node == to_node)
	{
		if (!from_node)
			slog(LG_ERROR, "%s: failed to find node %u in list (BUG)", MOWGLI_FUNC_NAME, from_id);
		else
			slog(LG_ERROR, "%s: trying to move node %u to position %u but found the same node (BUG)", MOWGLI_FUNC_NAME, from_id, to_id);

		command_fail(si, fault_internalerror, _("There was an error modifying the list. This is a bug in services."));
		return;
	}

	mowgli_node_delete(from_node, list);
	mowgli_node_add_before(entry, from_node, list, to_node);

	if (oper)
		command_success_nodata(si, _("Oper logon info entry \2%u\2 moved to position \2%u\2."), from_id, to_id);
	else
		command_success_nodata(si, _("Logon info entry \2%u\2 moved to position \2%u\2."), from_id, to_id);
	logcommand(si, CMDLOG_ADMIN, "INFO:%s: \2%u\2 -> \2%u\2: \2%s\2, \2%s\2", oper ? "OMOVE" : "MOVE", from_id, to_id, entry->subject, entry->story);
	return;
}

static void
is_cmd_move(struct sourceinfo *si, int parc, char *parv[])
{
	is_move_impl(si, parc, parv, false);
}

static void
is_cmd_omove(struct sourceinfo *si, int parc, char *parv[])
{
	is_move_impl(si, parc, parv, true);
}

static void
is_list_impl(struct sourceinfo *si, int parc, char *parv[], bool oper)
{
	mowgli_node_t *n;
	unsigned int x = 0;
	mowgli_list_t *list = oper ? &operlogon_info : &logon_info;

	MOWGLI_ITER_FOREACH(n, list->head)
	{
		struct logoninfo *l = n->data;
		x++;

		char *y = sstrdup(l->subject);
		underscores_to_spaces(y);

		if (logoninfo_count != 0 && has_priv(si, PRIV_GLOBAL) && list->count > logoninfo_count)
		{
			if (!logoninfo_reverse && x == logoninfo_count + 1)
				command_success_nodata(si, _("(Messages below this line are not shown by default)"));
			else if (logoninfo_reverse && x == list->count - logoninfo_count + 1)
				command_success_nodata(si, _("(Messages above this line are no longer shown by default)"));
		}

		if (logoninfo_show_metadata || has_priv(si, PRIV_GLOBAL))
		{
			struct tm *tm;
			tm = localtime(&l->info_ts);

			char dBuf[BUFSIZE];
			strftime(dBuf, BUFSIZE, "%H:%M on %m/%d/%Y", tm);

			command_success_nodata(si, _("%u: [\2%s\2] by \2%s\2 at \2%s\2: %s"),
				x, y, l->nick, dBuf, l->story);
		}
		else
		{
			command_success_nodata(si, _("[\2%s\2] %s"), y, l->story);
		}

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

static struct command is_move = {
	.name           = "MOVE",
	.desc           = N_("Move news items."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 2,
	.cmd            = &is_cmd_move,
	.help           = { .path = "infoserv/move" },
};

static struct command is_omove = {
	.name           = "OMOVE",
	.desc           = N_("Move oper news items."),
	.access         = PRIV_GLOBAL,
	.maxparc        = 2,
	.cmd            = &is_cmd_omove,
	.help           = { .path = "infoserv/omove" },
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

	struct infoserv_persist_record *rec = mowgli_global_storage_get(INFOSERV_PERSIST_STORAGE_NAME);

	if (rec)
	{
		mowgli_global_storage_free(INFOSERV_PERSIST_STORAGE_NAME);

		if (rec->version > INFOSERV_PERSIST_VERSION)
		{
			slog(LG_ERROR, "%s: attempted downgrade is not supported (from %u to %u)", m->name, rec->version, INFOSERV_PERSIST_VERSION);
			m->mflags |= MODFLAG_FAIL;

			sfree(rec);
			return;
		}

		logon_info     = rec->logon_info;
		operlogon_info = rec->operlogon_info;

		sfree(rec);
	}

	infoserv = service_add("infoserv", NULL);
	add_uint_conf_item("LOGONINFO_COUNT", &infoserv->conf_table, 0, &logoninfo_count, 0, INT_MAX, 3);
	add_bool_conf_item("LOGONINFO_REVERSE", &infoserv->conf_table, 0, &logoninfo_reverse, true);
	add_bool_conf_item("LOGONINFO_SHOW_METADATA", &infoserv->conf_table, 0, &logoninfo_show_metadata, true);

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
	service_bind_command(infoserv, &is_move);
	service_bind_command(infoserv, &is_omove);
	service_bind_command(infoserv, &is_list);
	service_bind_command(infoserv, &is_olist);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	struct infoserv_persist_record *rec = smalloc(sizeof *rec);
	rec->version        = INFOSERV_PERSIST_VERSION;
	rec->logon_info     = logon_info;
	rec->operlogon_info = operlogon_info;

	mowgli_global_storage_put(INFOSERV_PERSIST_STORAGE_NAME, rec);

	del_conf_item("LOGONINFO_COUNT", &infoserv->conf_table);
	del_conf_item("LOGONINFO_REVERSE", &infoserv->conf_table);
	del_conf_item("LOGONINFO_SHOW_METADATA", &infoserv->conf_table);

	hook_del_user_add(hook_user_add);
	hook_del_user_oper(hook_user_oper);
	hook_del_operserv_info(osinfo_hook);
	hook_del_db_write(write_infodb);

	db_unregister_type_handler(DB_TYPE_LOGONINFO);
	db_unregister_type_handler(DB_TYPE_LOGONINFO_OPER);

	service_delete(infoserv);
}

SIMPLE_DECLARE_MODULE_V1("infoserv/main", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
