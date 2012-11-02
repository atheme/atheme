/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the OpenSEX (Open Services Exchange) database backend for
 * Atheme. The purpose of OpenSEX is to destroy the old DB format, subjugate its
 * peoples, burn its cities to the ground, and salt the earth so that nothing
 * ever grows there again.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"backend/opensex", true, _modinit, NULL,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

typedef struct opensex_ {
	/* Lexing state */
	char *buf;
	unsigned int bufsize;
	char *token;
	FILE *f;

	/* Interpreting state */
	unsigned int grver;
} opensex_t;

static void opensex_db_parse(database_handle_t *db)
{
	const char *cmd;
	while (db_read_next_row(db))
	{
		cmd = db_read_word(db);
		if (!cmd || !*cmd || strchr("#\n\t \r", *cmd)) continue;
		db_process(db, cmd);
	}
}

static void opensex_h_grver(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	rs->grver = db_sread_int(db);
	slog(LG_INFO, "opensex: grammar version is %d.", rs->grver);
}

/***************************************************************************************************/

static bool opensex_read_next_row(database_handle_t *hdl)
{
	int c = 0;
	unsigned int n = 0;
	opensex_t *rs = (opensex_t *)hdl->priv;

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
		slog(LG_ERROR, "opensex-read-next-row: error at %s line %d: %s", hdl->file, hdl->line, strerror(errno));
		slog(LG_ERROR, "opensex-read-next-row: exiting to avoid data loss");
		exit(EXIT_FAILURE);
	}

	if (c == EOF && n == 0)
		return false;

	hdl->line++;
	hdl->token = 0;
	return true;
}

static const char *opensex_read_word(database_handle_t *db)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char *ptr = rs->token;
	char *res;
	static char buf[BUFSIZE];

	switch (rs->grver)
	{
	case 2:
		res = rs->token;

		ptr = strchr(res, '(');
		if (ptr != NULL)
		{
			char *bi, *pi;
			bool escaped = false;

			ptr++;
			for (bi = buf, pi = ptr; *pi != '\0' && (bi - buf) < sizeof buf; pi++)
			{
				switch (*pi)
				{
				case '\\':
					escaped = true;
					pi++;
					break;
				case ')':
					if (!escaped)
						goto demarshal_out;
				default:
					*bi++ = *pi;
					escaped = false;
					break;
				}
			}
demarshal_out:
			*bi++ = '\0';
			rs->token = pi;
			res = buf;
			slog(LG_DEBUG, "opensex_read_word(): read [%s], pi [%s]", res, pi);
		}
		else
			rs->token = NULL;

		break;
	case 1:
	default:
		res = rs->token;

		ptr = strchr(res, ' ');
		if (ptr != NULL)
		{
			*ptr++ = '\0';
			rs->token = ptr;
		}
		else
			rs->token = NULL;

		break;
	}

	db->token++;

	return res;
}

static const char *opensex_read_str(database_handle_t *db)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char *res;

	switch (rs->grver)
	{
	case 2:
		/*
		 * no special handling needed for strings verses words.  all words are strings,
		 * and all types derive from word, so all cells are encapsulated.
		 */
		return opensex_read_word(db);
	case 1:
	default:
		/* rs->token will be pointing at the next cell always, and in grammar version 1
		 * we just eat the remaining line if it's a multi-word cell. --nenolod
		 */
		res = rs->token;
		break;
	}

	db->token++;
	return res;
}

static bool opensex_read_int(database_handle_t *db, int *res)
{
	const char *s = db_read_word(db);
	char *rp;

	if (!s) return false;

	*res = strtol(s, &rp, 0);
	return *s && !*rp;
}

static bool opensex_read_uint(database_handle_t *db, unsigned int *res)
{
	const char *s = db_read_word(db);
	char *rp;

	if (!s) return false;

	*res = strtoul(s, &rp, 0);
	return *s && !*rp;
}

static bool opensex_read_time(database_handle_t *db, time_t *res)
{
	const char *s = db_read_word(db);
	char *rp;

	if (!s) return false;

	*res = strtoul(s, &rp, 0);
	return *s && !*rp;
}

static bool opensex_start_row(database_handle_t *db, const char *type)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	return_val_if_fail(type != NULL, false);
	rs = (opensex_t *)db->priv;

	switch (rs->grver)
	{
	case 2:
		fprintf(rs->f, "(%s)", type);
		break;
	case 1:
	default:
		fprintf(rs->f, "%s ", type);
		break;
	}

	return true;
}

static bool opensex_write_cell(database_handle_t *db, const char *data, bool multiword)
{
	opensex_t *rs;
	char buf[BUFSIZE], *bi;
	const char *i;

	return_val_if_fail(db != NULL, false);
	rs = (opensex_t *)db->priv;

	switch (rs->grver)
	{
	case 2:
		if (data == NULL)
		{
			fprintf(rs->f, "(*)");
			break;
		}

		bi = buf;
		*bi++ = '(';

		for (i = data; *i != '\0'; i++)
		{
			switch (*i)
			{
			case '(':
			case ')':
				*bi++ = '\\';
			default:
				*bi++ = *i;
				break;
			}
		}

		*bi++ = ')';
		*bi++ = '\0';

		fprintf(rs->f, "%s", buf);
		break;
	case 1:
	default:
		fprintf(rs->f, "%s%s", data != NULL ? data : "*", !multiword ? " " : "");
		break;
	}

	return true;
}

static bool opensex_write_word(database_handle_t *db, const char *word)
{
	return opensex_write_cell(db, word, false);
}

static bool opensex_write_str(database_handle_t *db, const char *word)
{
	return opensex_write_cell(db, word, true);
}

static bool opensex_write_int(database_handle_t *db, int num)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", num);
	return opensex_write_cell(db, buf, false);
}

static bool opensex_write_uint(database_handle_t *db, unsigned int num)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%u", num);
	return opensex_write_cell(db, buf, false);
}

static bool opensex_write_time(database_handle_t *db, time_t tm)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%lu", tm);
	return opensex_write_cell(db, buf, false);
}

static bool opensex_commit_row(database_handle_t *db)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	rs = (opensex_t *)db->priv;

	switch (rs->grver)
	{
	case 1:
	case 2:
	default:
		fprintf(rs->f, "\n");
		break;
	}

	return true;
}

static database_vtable_t opensex_vt = {
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

static database_handle_t *opensex_db_open_read(const char *filename)
{
	database_handle_t *db;
	opensex_t *rs;
	FILE *f;
	int errno1;
	char path[BUFSIZE];

	snprintf(path, BUFSIZE, "%s/%s", datadir, filename != NULL ? filename : "services.db");
	f = fopen(path, "r");
	if (!f)
	{
		errno1 = errno;

		/* ENOENT can happen if the database does not exist yet. */
		if (errno == ENOENT)
		{
			slog(LG_ERROR, "db-open-read: database '%s' does not yet exist; a new one will be created.", path);
			return NULL;
		}

		slog(LG_ERROR, "db-open-read: cannot open '%s' for reading: %s", path, strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: db-open-read: cannot open '%s' for reading: %s"), path, strerror(errno1));
		return NULL;
	}

	rs = scalloc(sizeof(opensex_t), 1);
	rs->grver = 1;
	rs->buf = scalloc(512, 1);
	rs->bufsize = 512;
	rs->token = NULL;
	rs->f = f;

	db = scalloc(sizeof(database_handle_t), 1);
	db->priv = rs;
	db->vt = &opensex_vt;
	db->txn = DB_READ;
	db->file = sstrdup(path);
	db->line = 0;
	db->token = 0;

	return db;
}

static database_handle_t *opensex_db_open_write(const char *filename)
{
	database_handle_t *db;
	opensex_t *rs;
	FILE *f;
	int errno1, grver;
	char bpath[BUFSIZE], path[BUFSIZE];

	snprintf(bpath, BUFSIZE, "%s/%s", datadir, filename != NULL ? filename : "services.db");

	mowgli_strlcpy(path, bpath, sizeof path);
	mowgli_strlcat(path, ".new", sizeof path);

	f = fopen(path, "w");
	if (!f)
	{
		errno1 = errno;
		slog(LG_ERROR, "db-open-write: cannot open '%s' for writing: %s", path, strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: db-open-write: cannot open '%s' for writing: %s"), path, strerror(errno1));
		return NULL;
	}

	rs = scalloc(sizeof(opensex_t), 1);
	rs->f = f;
	rs->grver = 1;
#ifndef EXPERIMENTAL
	grver = 1;
#else
	grver = 2;
#endif

	db = scalloc(sizeof(database_handle_t), 1);
	db->priv = rs;
	db->vt = &opensex_vt;
	db->txn = DB_WRITE;
	db->file = sstrdup(bpath);
	db->line = 0;
	db->token = 0;

	db_start_row(db, "GRVER");
	db_write_int(db, grver);
	db_commit_row(db);

	rs->grver = grver;

	return db;
}

static database_handle_t *opensex_db_open(const char *filename, database_transaction_t txn)
{
	if (txn == DB_WRITE)
		return opensex_db_open_write(filename);
	return opensex_db_open_read(filename);
}

static void opensex_db_close(database_handle_t *db)
{
	opensex_t *rs;
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
		/* now, replace the old database with the new one, using an atomic rename */
		if (srename(oldpath, newpath) < 0)
		{
			errno1 = errno;
			slog(LG_ERROR, "db_save(): cannot rename services.db.new to services.db: %s", strerror(errno1));
			wallops(_("\2DATABASE ERROR\2: db_save(): cannot rename services.db.new to services.db: %s"), strerror(errno1));
		}

		hook_call_db_saved();
	}

	free(rs->buf);
	free(rs);
	free(db->file);
	free(db);
}

static database_module_t opensex_mod = {
	.db_open = opensex_db_open,
	.db_close = opensex_db_close,
	.db_parse = opensex_db_parse,
};

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "backend/corestorage");

	m->mflags = MODTYPE_CORE;

	db_mod = &opensex_mod;

	db_register_type_handler("GRVER", opensex_h_grver);

	backend_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
