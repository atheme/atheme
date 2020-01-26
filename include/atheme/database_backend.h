/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
 *
 * Platform-agnostic database backend layer.
 */

#ifndef ATHEME_INC_DATABASE_BACKEND_H
#define ATHEME_INC_DATABASE_BACKEND_H 1

#include <atheme/attributes.h>
#include <atheme/constants.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

extern bool strict_mode;

enum database_transaction
{
	DB_READ,
	DB_WRITE
};

struct database_vtable
{
	const char *    name;

	/* Reading stuff. */
	bool          (*read_next_row)(struct database_handle *hdl);
	const char *  (*read_word)(struct database_handle *hdl);
	const char *  (*read_str)(struct database_handle *hdl);
	bool          (*read_int)(struct database_handle *hdl, int *res);
	bool          (*read_uint)(struct database_handle *hdl, unsigned int *res);
	bool          (*read_time)(struct database_handle *hdl, time_t *res);

	/* Writing stuff. */
	bool          (*start_row)(struct database_handle *hdl, const char *type);
	bool          (*write_word)(struct database_handle *hdl, const char *word);
	bool          (*write_str)(struct database_handle *hdl, const char *str);
	bool          (*write_int)(struct database_handle *hdl, int num);
	bool          (*write_uint)(struct database_handle *hdl, unsigned int num);
	bool          (*write_time)(struct database_handle *hdl, time_t time);
	bool          (*commit_row)(struct database_handle *hdl);
};

struct database_handle
{
	void *                          priv;
	const struct database_vtable *  vt;
	enum database_transaction       txn;
	char *                          file;
	unsigned int                    line;
	unsigned int                    token;
};

struct database_module
{
	struct database_handle *      (*db_open)(const char *filename, enum database_transaction txn);
	void                          (*db_close)(struct database_handle *db);
	void                          (*db_parse)(struct database_handle *db);
};

struct database_handle *db_open(const char *filename, enum database_transaction txn);
void db_close(struct database_handle *db);
void db_parse(struct database_handle *db);

bool db_read_next_row(struct database_handle *db);

const char *db_read_word(struct database_handle *db);
const char *db_read_str(struct database_handle *db);
bool db_read_int(struct database_handle *db, int *r);
bool db_read_uint(struct database_handle *db, unsigned int *r);
bool db_read_time(struct database_handle *db, time_t *t);

const char *db_sread_word(struct database_handle *db);
const char *db_sread_str(struct database_handle *db);
int db_sread_int(struct database_handle *db);
unsigned int db_sread_uint(struct database_handle *db);
time_t db_sread_time(struct database_handle *db);

bool db_start_row(struct database_handle *db, const char *type);
bool db_write_word(struct database_handle *db, const char *word);
bool db_write_str(struct database_handle *db, const char *str);
bool db_write_int(struct database_handle *db, int num);
bool db_write_uint(struct database_handle *db, unsigned int num);
bool db_write_time(struct database_handle *db, time_t time);
bool db_write_format(struct database_handle *db, const char *str, ...) ATHEME_FATTR_PRINTF(2, 3);
bool db_commit_row(struct database_handle *db);

typedef void (*database_handler_fn)(struct database_handle *db, const char *type);

void db_register_type_handler(const char *type, database_handler_fn fun);
void db_unregister_type_handler(const char *type);
void db_process(struct database_handle *db, const char *type);
void db_init(void);
extern const struct database_module *db_mod;

#endif /* !ATHEME_INC_DATABASE_BACKEND_H */
