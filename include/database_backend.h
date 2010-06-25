/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Platform-agnostic database backend layer.
 */

#ifndef DATABASE_BACKEND_H
#define DATABASE_BACKEND_H

#include "common.h"

struct database_handle_;
typedef struct database_handle_ database_handle_t;

typedef struct {
	const char *name;

	/* Reading stuff. */
	bool (*read_next_row)(database_handle_t *hdl);

	const char *(*read_word)(database_handle_t *hdl);
	const char *(*read_multiword)(database_handle_t *hdl);
	bool (*read_int)(database_handle_t *hdl, int *res);

	const char *(*sread_word)(database_handle_t *hdl);
	const char *(*sread_multiword)(database_handle_t *hdl);
	int (*sread_int)(database_handle_t *hdl);

	/* Writing stuff. */
	bool (*start_row)(database_handle_t *hdl, const char *type);
	bool (*write_word)(database_handle_t *hdl, const char *word);
	bool (*write_multiword)(database_handle_t *hdl, const char *str);
	bool (*write_int)(database_handle_t *hdl, int num);
	bool (*commit_row)(database_handle_t *hdl);
} database_vtable_t;

typedef enum {
	DB_READ,
	DB_WRITE
} database_transaction_t;

struct database_handle_ {
	void *priv;
	database_vtable_t *vt;
	database_transaction_t txn;
	const char *file;
	unsigned int line;
	unsigned int token;
};

typedef struct {
	database_handle_t *(*db_open)(database_transaction_t txn);
	void (*db_close)(database_handle_t *db);
} database_module_t;

E database_handle_t *db_open(database_transaction_t txn);
E void db_close(database_handle_t *db);

E bool db_read_next_row(database_handle_t *db);
E const char *db_read_word(database_handle_t *db);
E const char *db_read_multiword(database_handle_t *db);
E bool db_read_int(database_handle_t *db, int *r);
E const char *db_sread_word(database_handle_t *db);
E const char *db_sread_multiword(database_handle_t *db);
E int db_sread_int(database_handle_t *db);

E bool db_start_row(database_handle_t *db, const char *type);
E bool db_write_word(database_handle_t *db, const char *word);
E bool db_write_multiword(database_handle_t *db, const char *str);
E bool db_write_int(database_handle_t *db, int num);
E bool db_write_format(database_handle_t *db, const char *str, ...);
E bool db_commit_row(database_handle_t *db);

/*
 * Write multiple columns at the same time to the DB.
 * Terminate with NULL.
 */
E bool db_write(database_handle_t *db, ...);

typedef void (*database_handler_f)(database_handle_t *db, const char *type);

E void db_register_type_handler(const char *type, database_handler_f fun);
E void db_unregister_type_handler(const char *type);
E void db_process(database_handle_t *db, const char *type);
E void db_init(void);
E database_module_t *db_mod;

#endif
