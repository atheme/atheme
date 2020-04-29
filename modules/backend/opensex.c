/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 Atheme Project (http://atheme.org/)
 *
 * This file contains the OpenSEX (Open Services Exchange) database backend for
 * Atheme. The purpose of OpenSEX is to destroy the old DB format, subjugate its
 * peoples, burn its cities to the ground, and salt the earth so that nothing
 * ever grows there again.
 */

#include <atheme.h>

struct opensex
{
	// Lexing state
	char *buf;
	unsigned int bufsize;
	char *token;
	FILE *f;

	// Interpreting state
	unsigned int grver;
};

#ifdef HAVE_FLOCK
static int lockfd;
#endif

static void
opensex_db_parse(struct database_handle *db)
{
	const char *cmd;
	while (db_read_next_row(db))
	{
		cmd = db_read_word(db);
		if (!cmd || !*cmd || strchr("#\n\t \r", *cmd)) continue;
		db_process(db, cmd);
	}
}

static void
opensex_h_grver(struct database_handle *db, const char *type)
{
	struct opensex *rs = (struct opensex *)db->priv;
	rs->grver = db_sread_uint(db);
	slog(LG_INFO, "opensex: grammar version is %u.", rs->grver);

	if (rs->grver != 1)
		slog(LG_ERROR, "opensex: grammar version %u is unsupported.  dazed and confused, but trying to continue.", rs->grver);
}

static bool
opensex_read_next_row(struct database_handle *hdl)
{
	int c = 0;
	unsigned int n = 0;
	struct opensex *rs = (struct opensex *)hdl->priv;

	while ((c = getc(rs->f)) != EOF && c != '\n')
	{
		rs->buf[n++] = c;
		if (n == rs->bufsize)
		{
			rs->bufsize *= 2;
			rs->buf = srealloc(rs->buf, rs->bufsize);
		}
	}
	rs->buf[n] = '\0';
	rs->token = rs->buf;

	if (c == EOF && ferror(rs->f))
	{
		slog(LG_ERROR, "opensex-read-next-row: error at %s line %u: %s", hdl->file, hdl->line, strerror(errno));
		slog(LG_ERROR, "opensex-read-next-row: exiting to avoid data loss");
		exit(EXIT_FAILURE);
	}

	if (c == EOF && n == 0)
		return false;

	hdl->line++;
	hdl->token = 0;
	return true;
}

static const char *
opensex_read_word(struct database_handle *db)
{
	struct opensex *rs = (struct opensex *)db->priv;
	char *ptr;
	char *res;
	static char buf[BUFSIZE];

	res = rs->token;
	if (res == NULL)
		return NULL;

	ptr = strchr(res, ' ');
	if (ptr != NULL)
	{
		*ptr++ = '\0';
		rs->token = ptr;
	}
	else
		rs->token = NULL;

	db->token++;

	return res;
}

static const char *
opensex_read_str(struct database_handle *db)
{
	struct opensex *rs = (struct opensex *)db->priv;
	char *res;

	res = rs->token;

	db->token++;
	return res;
}

static bool
opensex_read_int(struct database_handle *db, int *res)
{
	const char *s = db_read_word(db);
	char *rp;

	if (!s) return false;

	*res = strtol(s, &rp, 0);
	return *s && !*rp;
}

static bool
opensex_read_uint(struct database_handle *db, unsigned int *res)
{
	const char *s = db_read_word(db);
	char *rp;

	if (!s) return false;

	*res = strtoul(s, &rp, 0);
	return *s && !*rp;
}

static bool
opensex_read_time(struct database_handle *db, time_t *res)
{
	const char *s = db_read_word(db);
	char *rp;

	if (!s) return false;

	*res = strtoul(s, &rp, 0);
	return *s && !*rp;
}

static bool
opensex_start_row(struct database_handle *db, const char *type)
{
	struct opensex *rs;

	return_val_if_fail(db != NULL, false);
	return_val_if_fail(type != NULL, false);
	rs = (struct opensex *)db->priv;

	fprintf(rs->f, "%s ", type);

	return true;
}

static bool
opensex_write_cell(struct database_handle *db, const char *data, bool multiword)
{
	struct opensex *rs;
	char buf[BUFSIZE], *bi;
	const char *i;

	return_val_if_fail(db != NULL, false);
	rs = (struct opensex *)db->priv;

	fprintf(rs->f, "%s%s", data != NULL ? data : "*", !multiword ? " " : "");

	return true;
}

static bool
opensex_write_word(struct database_handle *db, const char *word)
{
	return opensex_write_cell(db, word, false);
}

static bool
opensex_write_str(struct database_handle *db, const char *word)
{
	return opensex_write_cell(db, word, true);
}

static bool
opensex_write_int(struct database_handle *db, int num)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", num);
	return opensex_write_cell(db, buf, false);
}

static bool
opensex_write_uint(struct database_handle *db, unsigned int num)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%u", num);
	return opensex_write_cell(db, buf, false);
}

static bool
opensex_write_time(struct database_handle *db, time_t tm)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%lu", (unsigned long) tm);
	return opensex_write_cell(db, buf, false);
}

static bool
opensex_commit_row(struct database_handle *db)
{
	struct opensex *rs;

	return_val_if_fail(db != NULL, false);
	rs = (struct opensex *)db->priv;

	fprintf(rs->f, "\n");

	return true;
}

static const struct database_vtable opensex_vt = {
	.name = "opensex",
	.read_next_row = opensex_read_next_row,
	.read_word = opensex_read_word,
	.read_str = opensex_read_str,
	.read_int = opensex_read_int,
	.read_uint = opensex_read_uint,
	.read_time = opensex_read_time,
	.start_row = opensex_start_row,
	.write_word = opensex_write_word,
	.write_str = opensex_write_str,
	.write_int = opensex_write_int,
	.write_uint = opensex_write_uint,
	.write_time = opensex_write_time,
	.commit_row = opensex_commit_row
};

static struct database_handle * ATHEME_FATTR_MALLOC
opensex_db_open_read(const char *filename)
{
	struct database_handle *db;
	struct opensex *rs;
	FILE *f;
	int errno1;
	char path[BUFSIZE];

	snprintf(path, BUFSIZE, "%s/%s", datadir, filename != NULL ? filename : "services.db");
	f = fopen(path, "r");
	if (!f)
	{
		errno1 = errno;

		// ENOENT can happen if the database does not exist yet.
		if (errno == ENOENT)
		{
			if (database_create)
			{
				slog(LG_INFO, "db-open-read: database '%s' does not yet exist; a new one will be created.", path);
				return NULL;
			}
			else
			{
				slog(LG_ERROR, "db-open-read: database '%s' does not yet exist; please specify the -b option to create a new one.", path);
				exit(EXIT_FAILURE);
			}
		}

		slog(LG_ERROR, "db-open-read: cannot open '%s' for reading: %s", path, strerror(errno1));
		wallops("\2DATABASE ERROR\2: db-open-read: cannot open '%s' for reading: %s", path, strerror(errno1));
		exit(EXIT_FAILURE);
	}
	else if (database_create)
	{
		slog(LG_ERROR, "db-open-read: database '%s' already exists, but you specified the -b option to create a new one; please remove the old database first", path);
		exit(EXIT_FAILURE);
	}

	rs = smalloc(sizeof *rs);
	rs->grver = 1;
	rs->bufsize = 512;
	rs->buf = smalloc(rs->bufsize);
	rs->f = f;

	db = smalloc(sizeof *db);
	db->priv = rs;
	db->vt = &opensex_vt;
	db->txn = DB_READ;
	db->file = sstrdup(path);

	return db;
}

static struct database_handle * ATHEME_FATTR_MALLOC
opensex_db_open_write(const char *filename)
{
	struct database_handle *db;
	struct opensex *rs;
	int fd;
	FILE *f;
	int errno1;
	char bpath[BUFSIZE], path[BUFSIZE];
#ifdef HAVE_FLOCK
	char lpath[BUFSIZE];
#endif

	snprintf(bpath, BUFSIZE, "%s/%s", datadir, filename != NULL ? filename : "services.db");

	mowgli_strlcpy(path, bpath, sizeof path);
	mowgli_strlcat(path, ".new", sizeof path);

#ifdef HAVE_FLOCK
	mowgli_strlcpy(lpath, bpath, sizeof lpath);
	mowgli_strlcat(lpath, ".lock", sizeof lpath);

	lockfd = open(lpath, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	flock(lockfd, LOCK_EX);
#endif

	fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (fd < 0 || ! (f = fdopen(fd, "w")))
	{
		errno1 = errno;
		slog(LG_ERROR, "db-open-write: cannot open '%s' for writing: %s", path, strerror(errno1));
		wallops("\2DATABASE ERROR\2: db-open-write: cannot open '%s' for writing: %s", path, strerror(errno1));
#ifdef HAVE_FLOCK
		close(lockfd);
#endif
		return NULL;
	}

	rs = smalloc(sizeof *rs);
	rs->f = f;
	rs->grver = 1;

	db = smalloc(sizeof *db);
	db->priv = rs;
	db->vt = &opensex_vt;
	db->txn = DB_WRITE;
	db->file = sstrdup(bpath);

	db_start_row(db, "GRVER");
	db_write_uint(db, rs->grver);
	db_commit_row(db);

	return db;
}

static struct database_handle *
opensex_db_open(const char *filename, enum database_transaction txn)
{
	if (txn == DB_WRITE)
		return opensex_db_open_write(filename);
	return opensex_db_open_read(filename);
}

static void
opensex_db_close(struct database_handle *db)
{
	struct opensex *rs;
	int errno1;
	char oldpath[BUFSIZE], newpath[BUFSIZE];

	return_if_fail(db != NULL);
	rs = db->priv;

	mowgli_strlcpy(oldpath, db->file, sizeof oldpath);
	mowgli_strlcat(oldpath, ".new", sizeof oldpath);

	mowgli_strlcpy(newpath, db->file, sizeof newpath);

	fclose(rs->f);

	if (db->txn == DB_WRITE)
	{
		// now, replace the old database with the new one, using an atomic rename
		if (srename(oldpath, newpath) < 0)
		{
			errno1 = errno;
			slog(LG_ERROR, "db_save(): cannot rename services.db.new to services.db: %s", strerror(errno1));
			wallops("\2DATABASE ERROR\2: db_save(): cannot rename services.db.new to services.db: %s", strerror(errno1));
		}

		hook_call_db_saved();
#ifdef HAVE_FLOCK
		close(lockfd);
#endif
	}

	sfree(rs->buf);
	sfree(rs);
	sfree(db->file);
	sfree(db);
}

static const struct database_module opensex_mod = {
	.db_open = opensex_db_open,
	.db_close = opensex_db_close,
	.db_parse = opensex_db_parse,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "backend/corestorage")

	db_mod = &opensex_mod;

	db_register_type_handler("GRVER", opensex_h_grver);

	backend_loaded = true;

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("backend/opensex", MODULE_UNLOAD_CAPABILITY_NEVER)
