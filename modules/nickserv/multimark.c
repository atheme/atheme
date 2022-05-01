/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2014 Atheme Project (http://atheme.org/)
 *
 * Allows setting multiple marks on nicknames using nickserv mark
 * Do NOT use this in combination with nickserv/mark!
 */

#include <atheme.h>
#include "list.h"

#define MULTIMARK_PERSIST_MDNAME "atheme.nickserv.multimark"

struct multimark
{
	char *setter_uid;
	char *setter_name;
	char *restored_from_uid;
	char *restored_from_account;
	time_t time;
	unsigned int number;
	char *mark;
	mowgli_node_t node;
};

struct restored_mark
{
	char *account_uid;
	char *account_name;
	char *nick;
	char *setter_uid;
	char *setter_name;
	time_t time;
	char *mark;
	mowgli_node_t node;
};

static mowgli_patricia_t *restored_marks;

static inline mowgli_list_t *
multimark_list(struct myuser *mu)
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

static bool
multimark_match(const struct mynick *mn, const void *arg)
{
	const char *markpattern = (const char*)arg;
	struct myuser *mu = mn->owner;

	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	struct multimark *mm;

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

static bool
is_user_marked(struct myuser *mu)
{
	mowgli_list_t *l = multimark_list(mu);

	return MOWGLI_LIST_LENGTH(l) != 0;
}

static bool
is_marked(const struct mynick *mn, const void *arg)
{
	struct myuser *mu = mn->owner;

	return is_user_marked(mu);
}

static mowgli_list_t *
restored_mark_list(const char *nick)
{
	mowgli_list_t *l = mowgli_patricia_retrieve(restored_marks, nick);

	if (l == NULL)
	{
		l = mowgli_list_create();
		mowgli_patricia_add(restored_marks, nick, l);
	}

	return l;
}

static void
write_multimark_db(struct database_handle *db)
{
	mowgli_node_t *n;
	struct myuser *mu;
	struct myentity_iteration_state state;
	mowgli_list_t *l;
	struct myentity *mt;

	mowgli_patricia_iteration_state_t state2;
	mowgli_list_t *rml;
	struct restored_mark *rm;
	struct multimark *mm;

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

			db_write_word(db, mm->restored_from_account);

			db_write_uint(db, mm->time);
			db_write_uint(db, mm->number);
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
			db_write_word(db, rm->nick);
			db_write_word(db, rm->setter_uid);
			db_write_word(db, rm->setter_name);
			db_write_uint(db, rm->time);
			db_write_str(db, rm->mark);
			db_commit_row(db);
		}
	}
}

static void
db_h_mm(struct database_handle *db, const char *type)
{
	struct myuser *mu;
	mowgli_patricia_iteration_state_t state;
	mowgli_list_t *l;

	const char *account_uid = db_sread_word(db);
	const char *setter_uid = db_sread_word(db);
	const char *setter_name = db_sread_word(db);
	const char *restored_from_uid = db_sread_word(db);
	const char *restored_from_account = db_sread_word(db);
	time_t time = db_sread_uint(db);
	unsigned int number = db_sread_uint(db);
	const char *mark = db_sread_str(db);

	mu = myuser_find_uid(account_uid);

	l = multimark_list(mu);

	struct multimark *const mm = smalloc(sizeof *mm);

	mm->setter_uid = sstrdup(setter_uid);
	mm->setter_name = sstrdup(setter_name);
	mm->restored_from_account = sstrdup(restored_from_account);

	if (strcasecmp(restored_from_uid, "NULL"))
	{
		mm->restored_from_uid = sstrdup(restored_from_uid);
	}

	mm->time = time;
	mm->number = number;
	mm->mark = sstrdup(mark);

	mowgli_node_add(mm, &mm->node, l);
}

static void
db_h_rm(struct database_handle *db, const char *type)
{
	struct myuser *mu;
	mowgli_patricia_iteration_state_t state;

	const char *account_uid = db_sread_word(db);
	const char *account_name = db_sread_word(db);
	const char *nick = db_sread_word(db);
	const char *setter_uid = db_sread_word(db);
	const char *setter_name = db_sread_word(db);
	time_t time = db_sread_uint(db);
	const char *mark = db_sread_str(db);

	mowgli_list_t *l = restored_mark_list(nick);

	struct restored_mark *rm = smalloc(sizeof *rm);

	rm->account_uid = sstrdup(account_uid);
	rm->account_name = sstrdup(account_name);
	rm->nick = sstrdup(nick);
	rm->setter_uid = sstrdup(setter_uid);
	rm->setter_name = sstrdup(setter_name);
	rm->time = time;
	rm->mark = sstrdup(mark);

	mowgli_node_add(rm, &rm->node, l);
}

static unsigned int
get_multimark_max(struct myuser *mu)
{
	unsigned int max = 0;

	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	struct multimark *mm;

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

// Copy old style marks
static void
migrate_user(struct myuser *mu)
{
	mowgli_list_t *l;
	struct metadata *md;

	char *setter, *reason;
	struct myuser *setter_user;

	time_t time;

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

	struct multimark *const mm = smalloc(sizeof *mm);

	/* "Was MARKED by nick (actual_account)"
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
	mm->restored_from_account = NULL;

	mm->time = time;
	mm->number = get_multimark_max(mu);
	mm->mark = sstrdup(reason);

	mowgli_node_add(mm, &mm->node, l);

	// remove old style mark
	metadata_delete(mu, "private:mark:setter");
	metadata_delete(mu, "private:mark:reason");
	metadata_delete(mu, "private:mark:timestamp");
}

static void
migrate_all(struct sourceinfo *si)
{
	struct myentity_iteration_state state;
	struct myentity *mt;

	command_success_nodata(si, _("Migrating mark data..."));

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		struct myuser *mu = user(mt);
		migrate_user(mu);
	}

	command_success_nodata(si, _("Mark data migrated successfully."));
}

static void
nick_ungroup_hook(struct hook_user_req *hdata)
{
	struct myuser *mu = hdata->mu;
	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	struct multimark *mm;

	char *uid = entity(mu)->id;
	const char *nick = hdata->mn->nick;
	const char *account = entity(mu)->name;

	mowgli_list_t *rml = restored_mark_list(nick);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;

		struct restored_mark *const rm = smalloc(sizeof *rm);
		rm->account_uid = sstrdup(uid);
		rm->nick = sstrdup(nick);
		rm->account_name = sstrdup(account);
		rm->setter_uid = sstrdup(mm->setter_uid);
		rm->setter_name = sstrdup(mm->setter_name);
		rm->time = mm->time;
		rm->mark = sstrdup(mm->mark);

		mowgli_node_add(rm, &rm->node, rml);
	}
}

static void
account_drop_hook(struct myuser *mu)
{
	// Let's turn old marks to new marks at this point,
	// so that they are preserved.

	migrate_user(mu);

	mowgli_list_t *l = multimark_list(mu);

	mowgli_node_t *n;
	struct multimark *mm;

	char *uid = entity(mu)->id;
	const char *name = entity(mu)->name;
	mowgli_list_t *rml = restored_mark_list(name);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		mm = n->data;

		struct restored_mark *const rm = smalloc(sizeof *rm);
		rm->account_uid = sstrdup(uid);
		rm->nick = sstrdup(name);
		rm->account_name = sstrdup(name);
		rm->setter_uid = sstrdup(mm->setter_uid);
		rm->setter_name = sstrdup(mm->setter_name);
		rm->time = mm->time;
		rm->mark = sstrdup(mm->mark);

		mowgli_node_add(rm, &rm->node, rml);
	}
}

static void
account_register_hook(struct myuser *mu)
{
	mowgli_list_t *l;
	mowgli_node_t *n, *tn;

	struct restored_mark *rm;
	mowgli_list_t *rml;

	const char *name = entity(mu)->name;

	char *setter_name;
	struct myuser *setter;

	//Migrate any old-style marks that have already been restored at user
	//creation.

	migrate_user(mu);

	l = multimark_list(mu);
	rml = restored_mark_list(name);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, rml->head)
	{
		rm = n->data;

		struct multimark *const mm = smalloc(sizeof *mm);
		mm->setter_uid = sstrdup(rm->setter_uid);
		mm->setter_name = sstrdup(rm->setter_name);
		mm->restored_from_uid = rm->account_uid;
		mm->restored_from_account = rm->account_name;
		mm->time = rm->time;
		mm->number = get_multimark_max(mu);
		mm->mark = sstrdup(rm->mark);

		mowgli_node_add(mm, &mm->node, l);

		mowgli_node_delete(&rm->node, rml);
	}
}

static void
nick_group_hook(struct hook_user_req *hdata)
{
	struct myuser *smu = hdata->si->smu;
	mowgli_list_t *l;

	mowgli_node_t *n, *tn, *n2;
	struct multimark *mm2;

	mowgli_list_t *rml;
	struct restored_mark *rm;

	char *uid = entity(smu)->id;
	const char *name = hdata->mn->nick;

	migrate_user(smu);

	l = multimark_list(smu);
	rml = restored_mark_list(name);

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

		struct multimark *const mm = smalloc(sizeof *mm);
		mm->setter_uid = sstrdup(rm->setter_uid);
		mm->setter_name = sstrdup(rm->setter_name);
		mm->restored_from_uid = rm->account_uid;
		mm->restored_from_account = rm->account_name;
		mm->time = rm->time;
		mm->number = get_multimark_max(smu);
		mm->mark = sstrdup(rm->mark);

		mowgli_node_add(mm, &mm->node, l);
	}
}

static void
show_multimark(struct hook_user_req *hdata)
{
	mowgli_list_t *l;

	mowgli_node_t *n;
	struct multimark *mm;
	struct tm *tm;
	char time[BUFSIZE];

	struct myuser *setter;
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
		tm = localtime(&mm->time);
		strftime(time, sizeof time, TIME_FORMAT, tm);

		if ((setter = myuser_find_uid(mm->setter_uid)) != NULL)
		{
			setter_name = entity(setter)->name;
		}
		else
		{
			setter_name = mm->setter_name;
		}

		if (mm->restored_from_uid == NULL)
		{
			if (strcasecmp(setter_name, mm->setter_name))
			{
				command_success_nodata(
					hdata->si,
					_("Mark \2%u\2 set by \2%s\2 (%s) on \2%s\2: %s"),
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
					_("Mark \2%u\2 set by \2%s\2 on \2%s\2: %s"),
					mm->number,
					setter_name,
					time,
					mm->mark
				);
			}
		}
		else
		{
			if (strcasecmp(setter_name, mm->setter_name))
			{
				struct myuser *user;
				if ((user = myuser_find_uid(mm->restored_from_uid)) != NULL)
				{
					command_success_nodata(
						hdata->si,
						_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 (%s) on \2%s\2: %s"),
						mm->number,
						mm->restored_from_account,
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
						_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 by \2%s\2 (%s) on \2%s\2: %s"),
						mm->number,
						mm->restored_from_account,
						setter_name,
						mm->setter_name,
						time,
						mm->mark
					);
				}
			}
			else
			{
				struct myuser *user;
				if ((user = myuser_find_uid(mm->restored_from_uid)) != NULL)
				{
					command_success_nodata(
						hdata->si,
						_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 on \2%s\2: %s"),
						mm->number,
						mm->restored_from_account,
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
						_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 by \2%s\2 on \2%s\2: %s"),
						mm->number,
						mm->restored_from_account,
						setter_name,
						time,
						mm->mark
					);
				}
			}
		}
	}
}

static void
show_multimark_noexist(struct hook_info_noexist_req *hdata)
{
	const char *nick = hdata->nick;

	mowgli_node_t *n;
	struct restored_mark *rm;
	struct tm *tm;
	char time[BUFSIZE];

	struct myuser *setter;
	const char *setter_name;

	bool has_user_auspex;

	has_user_auspex = has_priv(hdata->si, PRIV_USER_AUSPEX);

	if (!has_user_auspex) {
		return;
	}

	mowgli_list_t *l = restored_mark_list(nick);

	MOWGLI_ITER_FOREACH(n, l->head)
	{
		rm = n->data;
		tm = localtime(&rm->time);
		strftime(time, sizeof time, TIME_FORMAT, tm);

		if ((setter = myuser_find_uid(rm->setter_uid)) != NULL)
		{
			setter_name = entity(setter)->name;
		}
		else
		{
			setter_name = rm->setter_name;
		}

		if (strcasecmp(setter_name, rm->setter_name))
		{
			command_success_nodata(
				hdata->si,
				_("\2%s\2 is not registered anymore, but was \2marked\2 by \2%s\2 (%s) on \2%s\2: %s"),
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
				_("\2%s\2 is not registered anymore, but was \2marked\2 by \2%s\2 on \2%s\2: %s"),
				nick,
				setter_name,
				time,
				rm->mark
			);
		}
	}
}

static void
multimark_needforce(struct hook_user_needforce *hdata)
{
	struct myuser *mu;
	bool marked;

	mu = hdata->mu;
	marked = is_user_marked(mu);

	hdata->allowed = !marked;
}

static void
ns_cmd_multimark(struct sourceinfo *si, int parc, char *parv[])
{
	const char *target = parv[0];
	const char *action = parv[1];
	const char *info = parv[2];
	struct myuser *mu;
	struct myuser_name *mun;
	mowgli_list_t *l;

	mowgli_node_t *n, *tn;
	struct multimark *mm;
	struct tm *tm;
	char time[BUFSIZE];

	struct myuser *setter;
	const char *setter_name;

	mowgli_list_t *rl;

	bool has_admin;

	if (target && !strcasecmp(target, "MIGRATE") && !action)
	{
		has_admin = has_priv(si, PRIV_ADMIN);

		if (!has_admin)
		{
			command_fail(si, fault_noprivs, _("You need the %s privilege for this operation."), PRIV_ADMIN);
		}
		else
		{
			migrate_all(si);
		}

		return;
	}

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, _("usage: MARK <target> <ADD|DEL|LIST|MIGRATE> [note]"));
		return;
	}

	if ((mu = myuser_find_ext(target)))
		target = entity(mu)->name;
	else if (strcasecmp(action, "LIST"))
	{
		command_fail(si, fault_nosuch_target, STR_IS_NOT_REGISTERED, target);
		return;
	}

	if (!strcasecmp(action, "ADD"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <target> ADD <note>"));
			return;
		}

		l = multimark_list(mu);

		mm = smalloc(sizeof *mm);
		mm->setter_uid = sstrdup(entity(si->smu)->id);
		mm->setter_name = sstrdup(entity(si->smu)->name);
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
			struct restored_mark *rm;

			if (rl->count == 0)
			{
				command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered, and was not marked."), target);
				return;
			}

			MOWGLI_ITER_FOREACH(n, rl->head)
			{
				rm = n->data;
				tm = localtime(&rm->time);
				strftime(time, sizeof time, TIME_FORMAT, tm);

				if ((setter = myuser_find_uid(rm->setter_uid)) != NULL)
				{
					setter_name = entity(setter)->name;
				}
				else
				{
					setter_name = rm->setter_name;
				}

				if (strcasecmp(setter_name, rm->setter_name))
				{
					command_success_nodata(
						si,
						_("\2%s\2 is not registered anymore, but was \2marked\2 by \2%s\2 (%s) on \2%s\2: %s"),
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
						_("\2%s\2 is not registered anymore, but was \2marked\2 by \2%s\2 on \2%s\2: %s"),
						target,
						setter_name,
						time,
						rm->mark
					);
				}
			}

			return;
		}

		command_success_nodata(si, _("Marks for \2%s\2:"), target);

		l = multimark_list(mu);

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			mm = n->data;
			tm = localtime(&mm->time);
			strftime(time, sizeof time, TIME_FORMAT, tm);

			if ((setter = myuser_find_uid(mm->setter_uid)) != NULL)
			{
				setter_name = entity(setter)->name;
			}
			else
			{
				setter_name = mm->setter_name;
			}

			if (mm->restored_from_uid == NULL)
			{
				if (strcasecmp(setter_name, mm->setter_name))
				{
					command_success_nodata(
						si,
						_("Mark \2%u\2 set by \2%s\2 (%s) on \2%s\2: %s"),
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
						_("Mark \2%u\2 set by \2%s\2 on \2%s\2: %s"),
						mm->number,
						setter_name,
						time,
						mm->mark
					);
				}
			}
			else
			{
				if (strcasecmp(setter_name, mm->setter_name))
				{
					struct myuser *user;
					if ((user = myuser_find_uid(mm->restored_from_uid)) != NULL)
					{
						command_success_nodata(
							si,
							_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 (%s) on \2%s\2: %s"),
							mm->number,
							mm->restored_from_account,
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
							_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 by \2%s\2 (%s) on \2%s\2: %s"),
							mm->number,
							mm->restored_from_account,
							setter_name,
							mm->setter_name,
							time,
							mm->mark
						);
					}
				}
				else
				{
					struct myuser *user;
					if ((user = myuser_find_uid(mm->restored_from_uid)) != NULL)
					{
						command_success_nodata(
							si,
							_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 (\2%s\2) by \2%s\2 on \2%s\2: %s"),
							mm->number,
							mm->restored_from_account,
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
							_("\2(Restored)\2 Mark \2%u\2 originally set on \2%s\2 by \2%s\2 on \2%s\2: %s"),
							mm->number,
							mm->restored_from_account,
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
		unsigned int num;

		enum {
			DELETE_ONE_MARK,
			DELETE_RESTORED_MARKS,
			DELETE_ALL_MARKS,
		} mode = DELETE_ONE_MARK;

		if (! info )
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, _("Usage: MARK <target> DEL <number>|RESTORED|ALL"));
			return;
		}

		if (! strcasecmp(info, "RESTORED") )
			mode = DELETE_RESTORED_MARKS;
		else if (! strcasecmp(info, "ALL") )
			mode = DELETE_ALL_MARKS;
		else if (string_to_uint(info, &num))
			mode = DELETE_ONE_MARK;
		else
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
			command_fail(si, fault_badparams, _("Usage: MARK <target> DEL <number>|RESTORED|ALL"));
			return;
		}

		unsigned int found = 0;

		l = multimark_list(mu);

		MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
		{
			mm = n->data;

			switch (mode)
			{
				case DELETE_ONE_MARK:
					if (mm->number != num)
						continue;
					break;

				case DELETE_ALL_MARKS:
					break;

				case DELETE_RESTORED_MARKS:
					if (mm->restored_from_uid == NULL)
						continue;
					break;
			}

			mowgli_node_delete(&mm->node, l);

			sfree(mm->setter_uid);
			sfree(mm->setter_name);
			sfree(mm->restored_from_uid);
			sfree(mm->restored_from_account);
			sfree(mm->mark);
			sfree(mm);

			found++;

			if (mode == DELETE_ONE_MARK)
				break;
		}

		if (found)
		{
			switch (mode)
			{
				case DELETE_ONE_MARK:
					command_success_nodata(si, _("The mark has been deleted."));
					logcommand(si, CMDLOG_ADMIN, "MARK:DEL: \2%s\2 \2%s\2", target, info);
					break;

				case DELETE_ALL_MARKS:
					command_success_nodata(si, _("All marks have been deleted."));
					logcommand(si, CMDLOG_ADMIN, "MARK:DEL:ALL: \2%s\2 (%u matches)", target, found);
					break;

				case DELETE_RESTORED_MARKS:
					command_success_nodata(si, _("All restored marks have been deleted."));
					logcommand(si, CMDLOG_ADMIN, "MARK:DEL:RESTORED: \2%s\2 (%u matches)", target, found);
					break;
			}
		}
		else
		{
			switch (mode)
			{
				case DELETE_ONE_MARK:
					command_fail(si, fault_nosuch_key, _("This mark does not exist."));
					break;

				case DELETE_ALL_MARKS:
					command_fail(si, fault_nosuch_key, _("This account has no marks."));
					break;

				case DELETE_RESTORED_MARKS:
					command_fail(si, fault_nosuch_key, _("This account has no restored marks."));
					break;
			}
		}
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_badparams, _("Usage: MARK <target> <ADD|DEL|LIST|MIGRATE> [note]"));
	}
}

static struct command ns_multimark = {
	.name           = "MARK",
	.desc           = N_("Adds a note to a user."),
	.access         = PRIV_MARK,
	.maxparc        = 3,
	.cmd            = &ns_cmd_multimark,
	.help           = { .path = "nickserv/multimark" },
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

	MODULE_CONFLICT(m, "nickserv/mark")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	use_nslist_main_symbols(m);

	if (! (restored_marks = mowgli_global_storage_get(MULTIMARK_PERSIST_MDNAME)))
		restored_marks = mowgli_patricia_create(&irccasecanon);
	else
		mowgli_global_storage_free(MULTIMARK_PERSIST_MDNAME);

	hook_add_db_write(write_multimark_db);
	db_register_type_handler("MM", db_h_mm);
	db_register_type_handler("RM", db_h_rm);

	hook_add_user_info(show_multimark);
	hook_add_user_info_noexist(show_multimark_noexist);
	hook_add_user_needforce(multimark_needforce);
	hook_add_user_drop(account_drop_hook);
	hook_add_nick_ungroup(nick_ungroup_hook);
	hook_add_nick_group(nick_group_hook);
	hook_add_user_register(account_register_hook);

	service_named_bind_command("nickserv", &ns_multimark);

	static struct list_param mark;
	mark.opttype = OPT_STRING;
	mark.is_match = multimark_match;

	list_register("mark-reason", &mark);

	static struct list_param mark_check;
	mark_check.opttype = OPT_BOOL;
	mark_check.is_match = is_marked;

	list_register("marked", &mark_check);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
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

	mowgli_global_storage_put(MULTIMARK_PERSIST_MDNAME, restored_marks);
}

SIMPLE_DECLARE_MODULE_V1("nickserv/multimark", MODULE_UNLOAD_CAPABILITY_RELOAD_ONLY)
