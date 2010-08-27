/* groupserv - group services.
 * Copyright (c) 2010 Atheme Development Group
 */

#include "groupserv.h"

#define GDBV_VERSION	1

static unsigned int loading_gdbv = -1;

static void write_groupdb(database_handle_t *db)
{
	myentity_t *me;
	myentity_iteration_state_t state;

	db_start_row(db, "GDBV");
	db_write_uint(db, GDBV_VERSION);
	db_commit_row(db);

	MYENTITY_FOREACH_T(me, &state, ENT_GROUP)
	{
		node_t *n;
		mygroup_t *mg = group(me);

		continue_if_fail(mg != NULL);

		db_start_row(db, "GRP");
		db_write_word(db, entity(mg)->name);
		db_write_time(db, mg->regtime);
		db_commit_row(db);

		LIST_FOREACH(n, mg->acs.head)
		{
			myuser_t *mu = user(n->data);

			continue_if_fail(mu != NULL);

			db_start_row(db, "GACL");
			db_start_row(db, entity(mg)->name);
			db_start_row(db, entity(mu)->name);
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

	mg = mygroup_add(name);
	mg->regtime = regtime;
}

static void db_h_gacl(database_handle_t *db, const char *type)
{
	mygroup_t *mg;
	myuser_t *mu;

	const char *name = db_sread_word(db);
	const char *user = db_sread_word(db);

	mg = mygroup_find(name);
	mu = myuser_find(user);

	if (mg == NULL)
		return;

	if (mu == NULL)
		return;

	node_add(mu, node_create(), &mg->acs);
}

void gs_db_init(void)
{
	hook_add_db_write(write_groupdb);

	db_register_type_handler("GDBV", db_h_gdbv);
	db_register_type_handler("GRP", db_h_grp);
	db_register_type_handler("GACL", db_h_gacl);
}

void gs_db_deinit(void)
{
	hook_del_db_write(write_groupdb);

	db_unregister_type_handler("GDBV");
	db_unregister_type_handler("GRP");
	db_unregister_type_handler("GACL");
}
