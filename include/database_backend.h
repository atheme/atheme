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

enum database_transaction
{
	DB_READ,
	DB_WRITE
};

struct database_vtable
{
	const char *name;

	/* Reading stuff. */
	bool (*read_next_row)(struct database_handle *hdl);

	const char *(*read_word)(struct database_handle *hdl);
	const char *(*read_str)(struct database_handle *hdl);
	bool (*read_int)(struct database_handle *hdl, int *res);
	bool (*read_uint)(struct database_handle *hdl, unsigned int *res);
	bool (*read_time)(struct database_handle *hdl, time_t *res);

	/* Writing stuff. */
	bool (*start_row)(struct database_handle *hdl, const char *type);
	bool (*write_word)(struct database_handle *hdl, const char *word);
	bool (*write_str)(struct database_handle *hdl, const char *str);
	bool (*write_int)(struct database_handle *hdl, int num);
	bool (*write_uint)(struct database_handle *hdl, unsigned int num);
	bool (*write_time)(struct database_handle *hdl, time_t time);
	bool (*commit_row)(struct database_handle *hdl);
};

struct database_handle
{
	void *priv;
	const struct database_vtable *vt;
	enum database_transaction txn;
	char *file;
	unsigned int line;
	unsigned int token;
};

struct database_module
{
	struct database_handle *(*db_open)(const char *filename, enum database_transaction txn);
	void (*db_close)(struct database_handle *db);
	void (*db_parse)(struct database_handle *db);
};

extern struct database_handle *db_open(const char *filename, enum database_transaction txn);
extern void db_close(struct database_handle *db);
extern void db_parse(struct database_handle *db);

extern bool db_read_next_row(struct database_handle *db);

extern const char *db_read_word(struct database_handle *db);
extern const char *db_read_str(struct database_handle *db);
extern bool db_read_int(struct database_handle *db, int *r);
extern bool db_read_uint(struct database_handle *db, unsigned int *r);
extern bool db_read_time(struct database_handle *db, time_t *t);

extern const char *db_sread_word(struct database_handle *db);
extern const char *db_sread_str(struct database_handle *db);
extern int db_sread_int(struct database_handle *db);
extern unsigned int db_sread_uint(struct database_handle *db);
extern time_t db_sread_time(struct database_handle *db);

extern bool db_start_row(struct database_handle *db, const char *type);
extern bool db_write_word(struct database_handle *db, const char *word);
extern bool db_write_str(struct database_handle *db, const char *str);
extern bool db_write_int(struct database_handle *db, int num);
extern bool db_write_uint(struct database_handle *db, unsigned int num);
extern bool db_write_time(struct database_handle *db, time_t time);
extern bool db_write_format(struct database_handle *db, const char *str, ...) ATHEME_FATTR_PRINTF(2, 3);
extern bool db_commit_row(struct database_handle *db);

typedef void (*database_handler_f)(struct database_handle *db, const char *type);

extern void db_register_type_handler(const char *type, database_handler_f fun);
extern void db_unregister_type_handler(const char *type);
extern void db_process(struct database_handle *db, const char *type);
extern void db_init(void);
extern const struct database_module *db_mod;

#endif
