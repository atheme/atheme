/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Platform-agnostic database backend layer.
 */

#ifndef DATABASE_BACKEND_H
#define DATABASE_BACKEND_H

#include "common.h"

extern bool strict_mode;

struct database_handle_;
typedef struct database_handle_ database_handle_t;

typedef struct {
	const char *name;

	/* Reading stuff. */
	bool (*read_next_row)(database_handle_t *hdl);

	const char *(*read_word)(database_handle_t *hdl);
	const char *(*read_str)(database_handle_t *hdl);
	bool (*read_int)(database_handle_t *hdl, int *res);
	bool (*read_uint)(database_handle_t *hdl, unsigned int *res);
	bool (*read_time)(database_handle_t *hdl, time_t *res);

	/* Writing stuff. */
	bool (*start_row)(database_handle_t *hdl, const char *type);
	bool (*write_word)(database_handle_t *hdl, const char *word);
	bool (*write_str)(database_handle_t *hdl, const char *str);
	bool (*write_int)(database_handle_t *hdl, int num);
	bool (*write_uint)(database_handle_t *hdl, unsigned int num);
	bool (*write_time)(database_handle_t *hdl, time_t time);
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
	char *file;
	unsigned int line;
	unsigned int token;
};

typedef struct {
	database_handle_t *(*db_open)(const char *filename, database_transaction_t txn);
	void (*db_close)(database_handle_t *db);
	void (*db_parse)(database_handle_t *db);
} database_module_t;

extern database_handle_t *db_open(const char *filename, database_transaction_t txn);
extern void db_close(database_handle_t *db);
extern void db_parse(database_handle_t *db);

extern bool db_read_next_row(database_handle_t *db);

extern const char *db_read_word(database_handle_t *db);
extern const char *db_read_str(database_handle_t *db);
extern bool db_read_int(database_handle_t *db, int *r);
extern bool db_read_uint(database_handle_t *db, unsigned int *r);
extern bool db_read_time(database_handle_t *db, time_t *t);

extern const char *db_sread_word(database_handle_t *db);
extern const char *db_sread_str(database_handle_t *db);
extern int db_sread_int(database_handle_t *db);
extern unsigned int db_sread_uint(database_handle_t *db);
extern time_t db_sread_time(database_handle_t *db);

extern bool db_start_row(database_handle_t *db, const char *type);
extern bool db_write_word(database_handle_t *db, const char *word);
extern bool db_write_str(database_handle_t *db, const char *str);
extern bool db_write_int(database_handle_t *db, int num);
extern bool db_write_uint(database_handle_t *db, unsigned int num);
extern bool db_write_time(database_handle_t *db, time_t time);
extern bool db_write_format(database_handle_t *db, const char *str, ...);
extern bool db_commit_row(database_handle_t *db);

typedef void (*database_handler_f)(database_handle_t *db, const char *type);

extern void db_register_type_handler(const char *type, database_handler_f fun);
extern void db_unregister_type_handler(const char *type);
extern void db_process(database_handle_t *db, const char *type);
extern void db_init(void);
extern database_module_t *db_mod;

#endif
