/* groupserv - group services.
 * Copyright (c) 2010 Atheme Development Group
 */

#include "groupserv.h"

#define GDBV_VERSION	3

static unsigned int loading_gdbv = -1;

static void write_groupdb(database_handle_t *db)
{
	myentity_t *mt;
	myentity_iteration_state_t state;

	db_start_row(db, "GDBV");
	db_write_uint(db, GDBV_VERSION);
	db_commit_row(db);

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mowgli_node_t *n;

		continue_if_fail(mt != NULL);
		mygroup_t *mg = group(mt);
		continue_if_fail(mg != NULL);

		char *mgflags = gflags_tostr(mg_flags, mg->flags);

		db_start_row(db, "GRP");
		db_write_word(db, entity(mg)->name);
		db_write_time(db, mg->regtime);
		db_write_word(db, mgflags);
		db_commit_row(db);

		MOWGLI_ITER_FOREACH(n, mg->acs.head)
		{
			groupacs_t *ga = n->data;
			char *flags = gflags_tostr(ga_flags, ga->flags);

			db_start_row(db, "GACL");
			db_start_row(db, entity(mg)->name);
			db_start_row(db, entity(ga->mu)->name);
			db_start_row(db, flags);
			db_commit_row(db);
		}

		MOWGLI_ITER_FOREACH(n, object(mg)->metadata.head)
		{
			metadata_t *md = (metadata_t *)n->data;

			db_start_row(db, "MDG");
			db_write_word(db, entity(mg)->name);
			db_write_word(db, md->name);
			db_write_str(db, md->value);
			db_commit_row(db);
		}
	}
}

static void db_h_gdbv(database_handle_t *db, const char *type)
{
	loading_gdbv = db_sread_uint(db);
	slog(LG_INFO, "groupserv: opensex data schema version is %d.", loading_gdbv);
}

static void db_h_grp(database_handle_t *db, const char *type)
{
	mygroup_t *mg;
	const char *name = db_sread_word(db);
	time_t regtime = db_sread_time(db);
	const char *flagset;

	mg = mygroup_add(name);
	mg->regtime = regtime;

	if (loading_gdbv >= 3)
	{
		flagset = db_sread_word(db);

		if (!gflags_fromstr(mg_flags, flagset, &mg->flags))
			slog(LG_INFO, "db-h-grp: line %d: confused by flags: %s", db->line, flagset);
	}
}

static void db_h_gacl(database_handle_t *db, const char *type)
{
	mygroup_t *mg;
	myuser_t *mu;
	unsigned int flags = GA_ALL;	/* GDBV 1 entires had full access */

	const char *name = db_sread_word(db);
	const char *user = db_sread_word(db);
	const char *flagset;

	mg = mygroup_find(name);
	mu = myuser_find(user);

	if (mg == NULL)
		return;

	if (mu == NULL)
		return;

	if (loading_gdbv >= 2)
	{
		flagset = db_sread_word(db);

		if (!gflags_fromstr(ga_flags, flagset, &flags))
			slog(LG_INFO, "db-h-gacl: line %d: confused by flags: %s", db->line, flagset);
	}

	groupacs_add(mg, mu, flags);
}

static void db_h_mdg(database_handle_t *db, const char *type)
{
	const char *name = db_sread_word(db);
	const char *prop = db_sread_word(db);
	const char *value = db_sread_str(db);
	void *obj = NULL;

	obj = mygroup_find(name);
	metadata_add(obj, prop, value);
}

void gs_db_init(void)
{
	hook_add_db_write_pre_ca(write_groupdb);

	db_register_type_handler("GDBV", db_h_gdbv);
	db_register_type_handler("GRP", db_h_grp);
	db_register_type_handler("GACL", db_h_gacl);
	db_register_type_handler("MDG", db_h_mdg);
}

void gs_db_deinit(void)
{
	hook_del_db_write_pre_ca(write_groupdb);

	db_unregister_type_handler("GDBV");
	db_unregister_type_handler("GRP");
	db_unregister_type_handler("GACL");
	db_unregister_type_handler("MDG");
}
