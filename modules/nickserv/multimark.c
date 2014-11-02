/*
 * Copyright (c) 2014 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows setting multiple marks on nicknames using nickserv mark
 * Do NOT use this in combination with nickserv/mark!
 */

#include "atheme.h"
#include "list.h"
#include "account.h"

DECLARE_MODULE_V1
(
	"nickserv/multimark", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_multimark(sourceinfo_t *si, int parc, char *parv[]);

static void write_multimark_db(database_handle_t *db);
static void db_h_mm(database_handle_t *db, const char *type);
static void db_h_rm(database_handle_t *db, const char *type);

static void show_multimark(hook_user_req_t *hdata);
static void show_multimark_noexist(hook_info_noexist_req_t *hdata);

static void account_drop_hook(myuser_t *mu);

static void nick_group_hook(hook_user_req_t *hdata);
static void nick_ungroup_hook(hook_user_req_t *hdata);
static void account_register_hook(myuser_t *mu);

static inline mowgli_list_t *multimark_list(myuser_t *mu);
int get_multimark_max(myuser_t *mu);

static mowgli_patricia_t *restored_marks;

command_t ns_multimark = { "MARK", N_("Adds a note to a user."), PRIV_MARK, 3, ns_cmd_multimark, { .path = "nickserv/multimark" } };

struct multimark {
	char *setter_uid;
	char *setter_name;
	char *restored_from_uid;
	time_t time;
	int number;
	char *mark;
	mowgli_node_t node;
};

struct restored_mark {
	char *account_uid;
	char *account_name;
	char *setter_uid;
	char *setter_name;
	time_t time;
	char *mark;
	mowgli_node_t node;
};

typedef struct multimark multimark_t;
typedef struct restored_mark restored_mark_t;

static bool multimark_match(const mynick_t *mn, const void *arg)
{
	const char *markpattern = (const char*)arg;
	myuser_t *mu = mn->owner;

	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	multimark_t *mm;

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;

		if (!match(markpattern, mm->mark))
		{
			return true;
		}
	}

	return false;
}

static bool is_marked(const mynick_t *mn, const void *arg)
{
	myuser_t *mu = mn->owner;
	mowgli_list_t *l = multimark_list(mu);

	return MOWGLI_LIST_LENGTH(l) != 0;
}

void _modinit(module_t *m)
{
	static list_param_t mark_check;

	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	if (module_find_published("nickserv/mark"))
	{
		slog(LG_INFO, "Loading both multimark and mark has severe consequences for the space-time continuum. Refusing to load.");
		m->mflags = MODTYPE_FAIL;
		return;
	}

	restored_marks = mowgli_patricia_create(strcasecanon);

	hook_add_db_write(write_multimark_db);
	db_register_type_handler("MM", db_h_mm);
	db_register_type_handler("RM", db_h_rm);

	hook_add_event("user_info");
	hook_add_user_info(show_multimark);

	hook_add_event("user_info_noexist");
	hook_add_user_info_noexist(show_multimark_noexist);

	hook_add_event("user_drop");
	hook_add_user_drop(account_drop_hook);

	hook_add_event("nick_ungroup");
	hook_add_nick_ungroup(nick_ungroup_hook);

	hook_add_event("nick_group");
	hook_add_nick_group(nick_group_hook);

	hook_add_event("user_register");
	hook_add_user_register(account_register_hook);

	service_named_bind_command("nickserv", &ns_multimark);

	use_nslist_main_symbols(m);

	static list_param_t mark;
	mark.opttype = OPT_STRING;
	mark.is_match = multimark_match;

	list_register("mark-reason", &mark);

	mark_check.opttype = OPT_BOOL;
	mark_check.is_match = is_marked;

	list_register("marked", &mark_check);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_db_write(write_multimark_db);
	db_unregister_type_handler("MM");
	db_unregister_type_handler("RM");
	db_unregister_type_handler("MDU");

	hook_del_user_info(show_multimark);
	hook_del_user_info_noexist(show_multimark_noexist);
	hook_del_user_drop(account_drop_hook);
	hook_del_nick_ungroup(nick_ungroup_hook);
	hook_del_nick_group(nick_group_hook);
	hook_del_user_register(account_register_hook);

	service_named_unbind_command("nickserv", &ns_multimark);

	list_unregister("mark-reason");
	list_unregister("marked");
}

static inline mowgli_list_t *multimark_list(myuser_t *mu)
{
	mowgli_list_t *l;

	return_val_if_fail(mu != NULL, NULL);

	l = privatedata_get(mu, "mark:list");

	if (l != NULL)
		return l;

	l = mowgli_list_create();
	privatedata_set(mu, "mark:list", l);

	return l;
}

mowgli_list_t *restored_mark_list(const char *nick)
{
	mowgli_list_t *l = mowgli_patricia_retrieve(restored_marks, nick);

	if (l == NULL)
	{
		l = mowgli_list_create();
		mowgli_patricia_add(restored_marks, nick, l);
	}

	return l;
}

static void write_multimark_db(database_handle_t *db)
{
	mowgli_node_t *n;
	myuser_t *mu;
	myentity_iteration_state_t state;
	mowgli_list_t *l;
	myentity_t *mt;

	mowgli_patricia_iteration_state_t state2;
	mowgli_list_t *rml;
	restored_mark_t *rm;
	multimark_t *mm;

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		mu = user(mt);
		l = multimark_list(mu);

		if (l == NULL)
		{
			continue;
		}

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			mm = n->data;
			db_start_row(db, "MM");
			db_write_word(db, entity(mu)->id);
			db_write_word(db, mm->setter_uid);
			db_write_word(db, mm->setter_name);

			if (mm->restored_from_uid == NULL)
			{
				db_write_word(db, "NULL");
			}
			else
			{
				db_write_word(db, mm->restored_from_uid);
			}

			db_write_uint(db, mm->time);
			db_write_int(db, mm->number);
			db_write_str(db, mm->mark);
			db_commit_row(db);
		}
	}

	MOWGLI_PATRICIA_FOREACH(rml, &state2, restored_marks)
	{
		MOWGLI_ITER_FOREACH(n, rml->head)
		{
			rm = n->data;
			db_start_row(db, "RM");
			db_write_word(db, rm->account_uid);
			db_write_word(db, rm->account_name);
			db_write_word(db, rm->setter_uid);
			db_write_word(db, rm->setter_name);
			db_write_uint(db, rm->time);
			db_write_str(db, rm->mark);
			db_commit_row(db);
		}
	}
}

static void db_h_mm(database_handle_t *db, const char *type)
{
	myuser_t *mu;
	mowgli_patricia_iteration_state_t state;
	mowgli_list_t *l;

	const char *account_uid = db_sread_word(db);
	const char *setter_uid = db_sread_word(db);
	const char *setter_name = db_sread_word(db);
	const char *restored_from_uid = db_sread_word(db);
	time_t time = db_sread_uint(db);
	int number = db_sread_int(db);
	const char *mark = db_sread_str(db);

	mu = myuser_find_uid(account_uid);

	l = multimark_list(mu);

	multimark_t *mm = smalloc(sizeof(multimark_t));

	mm->setter_uid = sstrdup(setter_uid);
	mm->setter_name = sstrdup(setter_name);
	mm->restored_from_uid = sstrdup(restored_from_uid);

	if (!strcasecmp (mm->restored_from_uid, "NULL"))
	{
		mm->restored_from_uid = NULL;
	}

	mm->time = time;
	mm->number = number;
	mm->mark = sstrdup(mark);

	mowgli_node_add(mm, &mm->node, l);
}

static void db_h_rm(database_handle_t *db, const char *type)
{
	myuser_t *mu;
	mowgli_patricia_iteration_state_t state;

	const char *account_uid = db_sread_word(db);
	const char *account_name = db_sread_word(db);
	const char *setter_uid = db_sread_word(db);
	const char *setter_name = db_sread_word(db);
	time_t time = db_sread_uint(db);
	const char *mark = db_sread_str(db);

	mowgli_list_t *l = restored_mark_list(account_name);

	restored_mark_t *rm = smalloc(sizeof(restored_mark_t));

	rm->account_uid = sstrdup(account_uid);
	rm->account_name = sstrdup(account_name);
	rm->setter_uid = sstrdup(setter_uid);
	rm->setter_name = sstrdup(setter_name);
	rm->time = time;
	rm->mark = sstrdup(mark);

	mowgli_node_add(rm, &rm->node, l);

	mowgli_patricia_add(restored_marks, account_name, l);
}

/* Copy old style marks */
static void migrate_user(myuser_t *mu)
{
	mowgli_list_t *l;
	metadata_t *md;

	char *setter, *reason;
	myuser_t *setter_user;

	time_t time;
	multimark_t *mm;

	l = multimark_list(mu);
	md = metadata_find(mu, "private:mark:setter");
	char *begin, *end;

	if (md == NULL) {
		return;
	}

	setter = md->value;

	md = metadata_find(mu, "private:mark:reason");
	reason = md != NULL ? md->value : "unknown";
	md = metadata_find(mu, "private:mark:timestamp");
	time = md != NULL ? atoi(md->value) : 0;

	mm = smalloc(sizeof(multimark_t));

	/*
	 * "Was MARKED by nick (actual_account)"
	 * Finds the string between '(' and ')', which
	 * is the account name
	 */
	begin = strchr(setter, '(');

	if (begin) {
		end = strchr(setter, ')');

		if (end) {
			*end = 0;
		}

		setter = sstrdup(begin+1);
	}

	if ((setter_user = myuser_find(setter)) != NULL) {
		mm->setter_uid = sstrdup(entity(setter_user)->id);
	} else {
		mm->setter_uid = NULL;
	}

	mm->setter_name = sstrdup(setter);
	mm->restored_from_uid = NULL;

	mm->time = time;
	mm->number = get_multimark_max(mu);
	mm->mark = sstrdup(reason);

	mowgli_node_add(mm, &mm->node, l);

	/* remove old style mark */
	metadata_delete(mu, "private:mark:setter");
	metadata_delete(mu, "private:mark:reason");
	metadata_delete(mu, "private:mark:timestamp");
}


int get_multimark_max(myuser_t *mu)
{
	int max = 0;

	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	multimark_t *mm;

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;
		if (mm->number > max)
		{
			max = mm->number;
		}
	}

	return max+1;
}

static void nick_ungroup_hook(hook_user_req_t *hdata)
{
	myuser_t *mu = hdata->mu;
	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	multimark_t *mm;

	char *uid = entity(mu)->id;
	const char *name = hdata->mn->nick;

	mowgli_list_t *rml = restored_mark_list(name);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;

		restored_mark_t *rm = smalloc(sizeof(restored_mark_t));
		rm->account_uid = sstrdup(uid);
		rm->account_name = sstrdup(name);
		rm->setter_uid = sstrdup(mm->setter_uid);
		rm->setter_name = sstrdup(mm->setter_name);
		rm->time = mm->time;
		rm->mark = sstrdup(mm->mark);

		mowgli_node_add(rm, &rm->node, rml);
	}

	mowgli_patricia_add(restored_marks, name, rml);
}

static void account_drop_hook(myuser_t *mu)
{
	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	multimark_t *mm;

	char *uid = entity(mu)->id;
	const char *name = entity(mu)->name;
	mowgli_list_t *rml = restored_mark_list(name);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;

		restored_mark_t *rm = smalloc(sizeof(restored_mark_t));
		rm->account_uid = sstrdup(uid);
		rm->account_name = sstrdup(name);
		rm->setter_uid = sstrdup(mm->setter_uid);
		rm->setter_name = sstrdup(mm->setter_name);
		rm->time = mm->time;
		rm->mark = sstrdup(mm->mark);

		mowgli_node_add(rm, &rm->node, rml);
	}

	mowgli_patricia_add(restored_marks, name, rml);
}

static void account_register_hook(myuser_t *mu)
{
	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n, *tn;
	restored_mark_t *rm;

	const char *name = entity(mu)->name;

	char *setter_name;
	myuser_t *setter;

	mowgli_list_t *rml = restored_mark_list(name);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rml->head)
	{
		rm = n->data;

		multimark_t *mm = smalloc(sizeof(multimark_t));

		mm->setter_uid = sstrdup(rm->setter_uid);
		mm->setter_name = sstrdup(rm->setter_name);
		mm->restored_from_uid = rm->account_uid;
		mm->time = rm->time;
		mm->number = get_multimark_max(mu);
		mm->mark = sstrdup(rm->mark);

		mowgli_node_add(mm, &mm->node, l);

		mowgli_node_delete(&rm->node, rml);
	}

	mowgli_patricia_add(restored_marks, name, rml);
}

static void nick_group_hook(hook_user_req_t *hdata)
{
	myuser_t *smu = hdata->si->smu;
	mowgli_list_t *l = multimark_list(smu);

	mowgli_node_t *n, *tn, *n2;
	multimark_t *mm2;
	restored_mark_t *rm;

	char *uid = entity(smu)->id;
	const char *name = hdata->mn->nick;

	mowgli_list_t *rml = restored_mark_list(name);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rml->head)
	{
		rm = n->data;
		bool already_exists = false;

		MOWGLI_ITER_FOREACH (n2, l->head)
		{
			mm2 = n2->data;

			if (!strcasecmp (mm2->mark, rm->mark))
			{
				already_exists = true;
				break;
			}
		}

		mowgli_node_delete(&rm->node, rml);

		if (already_exists)
		{
			continue;
		}

		multimark_t *mm = smalloc(sizeof(multimark_t));

		mm->setter_uid = sstrdup(rm->setter_uid);
		mm->setter_name = sstrdup(rm->setter_name);
		mm->restored_from_uid = rm->account_uid;
		mm->time = rm->time;
		mm->number = get_multimark_max(smu);
		mm->mark = sstrdup(rm->mark);

		mowgli_node_add(mm, &mm->node, l);
	}

	mowgli_patricia_add(restored_marks, name, rml);
}

static void show_multimark(hook_user_req_t *hdata)
{
	mowgli_list_t *l;

	mowgli_node_t *n;
	multimark_t *mm;
	struct tm tm;
	char time[BUFSIZE];

	myuser_t *setter;
	const char *setter_name;

	bool has_user_auspex;

	has_user_auspex = has_priv(hdata->si, PRIV_USER_AUSPEX);

	if (!has_user_auspex) {
		return;
	}

	migrate_user(hdata->mu);
	l = multimark_list(hdata->mu);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;
		tm = *localtime(&mm->time);

		strftime(time, sizeof time, TIME_FORMAT, &tm);

		if ( setter = myuser_find_uid(mm->setter_uid) )
		{
			setter_name = entity(setter)->name;
		}
		else
		{
			setter_name = mm->setter_name;
		}

		if ( mm->restored_from_uid == NULL )
		{
			if ( strcasecmp (setter_name, mm->setter_name) )
			{
				command_success_nodata(
					hdata->si,
					_("Mark \2%d\2 set by \2%s\2 (%s) on \2%s\2: %s"),
					mm->number,
					mm->setter_name,
					setter_name,
					time,
					mm->mark
				);
			}
			else
			{
				command_success_nodata(
					hdata->si,
					_("Mark \2%d\2 set by \2%s\2 on \2%s\2: %s"),
					mm->number,
					setter_name,
					time,
					mm->mark
				);
			}
		}
		else
		{
			if ( strcasecmp (setter_name, mm->setter_name) )
			{
				myuser_t *user;
				if (user = myuser_find_uid(mm->restored_from_uid))
				{
					command_success_nodata(
						hdata->si,
						_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 (%s) on \2%s\2: %s"),
						mm->number,
						mm->restored_from_uid,
						entity(user)->name,
						setter_name,
						mm->setter_name,
						time,
						mm->mark
					);
				}
				else
				{
					command_success_nodata(
						hdata->si,
						_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 by \2%s\2 (%s) on \2%s\2: %s"),
						mm->number,
						mm->restored_from_uid,
						setter_name,
						mm->setter_name,
						time,
						mm->mark
					);
				}
			}
			else
			{
				myuser_t *user;
				if (user = myuser_find_uid(mm->restored_from_uid))
				{
					command_success_nodata(
						hdata->si,
						_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 on \2%s\2: %s"),
						mm->number,
						mm->restored_from_uid,
						entity(user)->name,
						setter_name,
						time,
						mm->mark
					);
				}
				else
				{
					command_success_nodata(
						hdata->si,
						_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 by \2%s\2 on \2%s\2: %s"),
						mm->number,
						mm->restored_from_uid,
						setter_name,
						time,
						mm->mark
					);
				}
			}
		}
	}
}

static void show_multimark_noexist(hook_info_noexist_req_t *hdata)
{
	const char *nick = hdata->nick;

	mowgli_node_t *n;
	restored_mark_t *rm;
	struct tm tm;
	char time[BUFSIZE];

	myuser_t *setter;
	const char *setter_name;

	mowgli_list_t *l = restored_mark_list(nick);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		rm = n->data;
		tm = *localtime(&rm->time);

		strftime(time, sizeof time, TIME_FORMAT, &tm);

		if ( setter = myuser_find_uid(rm->setter_uid) )
		{
			setter_name = entity(setter)->name;
		}
		else
		{
			setter_name = rm->setter_name;
		}

		if ( strcasecmp (setter_name, rm->setter_name) )
		{
			command_success_nodata(
				hdata->si,
				_("\2%s\2 is not registered anymore but was \2marked\2 by \2%s\2 (%s) on \2%s\2: %s"),
				nick,
				setter_name,
				rm->setter_name,
				time,
				rm->mark
			);
		}
		else
		{
			command_success_nodata(
				hdata->si,
				_("\2%s\2 is not registered anymore but was \2marked\2 by \2%s\2 on \2%s\2: %s"),
				nick,
				setter_name,
				time,
				rm->mark
			);
		}
	}
}

static void ns_cmd_multimark(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	myuser_t *mu;
	myuser_name_t *mun;
	mowgli_list_t *l;

	mowgli_node_t *n;
	multimark_t *mm;
	struct tm tm;
	char time[BUFSIZE];

	myuser_t *setter;
	const char *setter_name;

	mowgli_list_t *rl;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("usage: MARK <target> <ADD|DEL|LIST> [note]"));
		return;
	}

	if (!(mu = myuser_find_ext(target)) && strcasecmp(action, "LIST"))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	l = multimark_list(mu);

	if (!strcasecmp(action, "ADD"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <target> ADD <note>"));
			return;
		}

		mm = smalloc(sizeof(multimark_t));
		mm->setter_uid = sstrdup(entity(si->smu)->id);
		mm->setter_name = sstrdup(entity(si->smu)->name);
		mm->restored_from_uid = NULL;
		mm->time = CURRTIME;
		mm->number = get_multimark_max(mu);
		mm->mark = sstrdup(info);
		mowgli_node_add(mm, &mm->node, l);

		command_success_nodata(si, _("\2%s\2 has been marked."), target);
		logcommand(si, CMDLOG_ADMIN, "MARK:ADD: \2%s\2 \2%s\2", target, info);
	}
	else if (!strcasecmp(action, "LIST"))
	{
		if (!mu)
		{
			rl = restored_mark_list(target);
			restored_mark_t *rm;

			if (rl->count == 0)
			{
				command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered, and was not marked."), target);
				return;
			}

			MOWGLI_ITER_FOREACH(n, rl->head)
			{
				rm = n->data;
				tm = *localtime(&rm->time);

				strftime(time, sizeof time, TIME_FORMAT, &tm);

				if ( setter = myuser_find_uid(rm->setter_uid) )
				{
					setter_name = entity(setter)->name;
				}
				else
				{
					setter_name = rm->setter_name;
				}

				if ( strcasecmp (setter_name, rm->setter_name) )
				{
					command_success_nodata(
						si,
						_("\2%s\2 is not registered anymore but was \2marked\2 by \2%s\2 (%s) on \2%s\2: %s"),
						target,
						setter_name,
						rm->setter_name,
						time,
						rm->mark
					);
				}
				else
				{
					command_success_nodata(
						si,
						_("\2%s\2 is not registered anymore but was \2marked\2 by \2%s\2 on \2%s\2: %s"),
						target,
						setter_name,
						time,
						rm->mark
					);
				}
			}

			return;
		}

		command_success_nodata(si, _("\2%s\2's marks:"), target);

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			mm = n->data;
			tm = *localtime(&mm->time);

			strftime(time, sizeof time, TIME_FORMAT, &tm);

			if ( setter = myuser_find_uid(mm->setter_uid) )
			{
				setter_name = entity(setter)->name;
			}
			else
			{
				setter_name = mm->setter_name;
			}

			if ( mm->restored_from_uid == NULL )
			{
				if ( strcasecmp (setter_name, mm->setter_name) )
				{
					command_success_nodata(
						si,
						_("Mark \2%d\2 set by \2%s\2 (%s) on \2%s\2: %s"),
						mm->number,
						mm->setter_name,
						setter_name,
						time,
						mm->mark
					);
				}
				else
				{
					command_success_nodata(
						si,
						_("Mark \2%d\2 set by \2%s\2 on \2%s\2: %s"),
						mm->number,
						setter_name,
						time,
						mm->mark
					);
				}
			}
			else
			{
				if ( strcasecmp (setter_name, mm->setter_name) )
				{
					myuser_t *user;
					if (user = myuser_find_uid(mm->restored_from_uid))
					{
						command_success_nodata(
							si,
							_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 (%s) on \2%s\2: %s"),
							mm->number,
							mm->restored_from_uid,
							entity(user)->name,
							setter_name,
							mm->setter_name,
							time,
							mm->mark
						);
					}
					else
					{
						command_success_nodata(
							si,
							_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 by \2%s\2 (%s) on \2%s\2: %s"),
							mm->number,
							mm->restored_from_uid,
							setter_name,
							mm->setter_name,
							time,
							mm->mark
						);
					}
				}
				else
				{
					myuser_t *user;
					if (user = myuser_find_uid(mm->restored_from_uid))
					{
						command_success_nodata(
							si,
							_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 on \2%s\2: %s"),
							mm->number,
							mm->restored_from_uid,
							entity(user)->name,
							setter_name,
							time,
							mm->mark
						);
					}
					else
					{
						command_success_nodata(
							si,
							_("\2(Restored)\2 Mark \2%d\2 originally set on \2%s\2 by \2%s\2 on \2%s\2: %s"),
							mm->number,
							mm->restored_from_uid,
							setter_name,
							time,
							mm->mark
						);
					}
				}
			}
		}

		command_success_nodata(si, _("End of list."));
	}
	else if (!strcasecmp(action, "DEL") || !strcasecmp(action, "DELETE"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <target> DEL <number>"));
			return;
		}

		bool found = false;
		int num = atoi(info);

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			mm = n->data;

			if ( mm->number == num )
			{
				mowgli_node_delete(&mm->node, l);

				free(mm->setter_uid);
				free(mm->setter_name);
				free(mm->restored_from_uid);
				free(mm->mark);
				free(mm);

				found = true;
				break;
			}
		}

		if (found)
		{
			command_success_nodata(si, _("The mark has been deleted."));
			logcommand(si, CMDLOG_ADMIN, "MARK:DEL: \2%s\2 \2%s\2", target, info);
		}
		else
		{
			command_fail(si, fault_nosuch_key, _("This mark does not exist"));
		}
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("usage: MARK <target> <ADD|DEL|LIST|MIGRATE> [note]"));
	}
}
