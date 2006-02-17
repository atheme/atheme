/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the database
 * using PostgreSQL.
 *
 * $Id: postgresql.c 4839 2006-02-17 23:37:21Z jilles $
 */

#include "atheme.h"
#include <libpq-fe.h>

DECLARE_MODULE_V1
(
	"backend/postgresql", TRUE, _modinit, NULL,
	"$Id: postgresql.c 4839 2006-02-17 23:37:21Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

PGconn *pq = NULL;
static int db_errored = 0;

static void db_connect(boolean_t startup)
{
	char dbcredentials[BUFSIZE];	

	db_errored = 0;

	if (pq == NULL)
	{
		if (!database_options.port)
			snprintf(dbcredentials, BUFSIZE, "host=%s dbname=%s user=%s password=%s",
					database_options.host, database_options.database, database_options.user, database_options.pass);
		else
			snprintf(dbcredentials, BUFSIZE, "host=%s dbname=%s user=%s password=%s port=%d",
					database_options.host, database_options.database, database_options.user, database_options.pass,
					database_options.port);

		slog(LG_DEBUG, "Connecting to PostgreSQL provider using these credentials:");
		slog(LG_DEBUG, "      %s", dbcredentials); 

		/* Open the db connection up. */
		pq = PQconnectdb(dbcredentials);

		if (PQstatus(pq) != CONNECTION_OK)
		{
			slog(LG_ERROR, "There was an error connecting to the database system:");
			slog(LG_ERROR, "      %s", PQerrorMessage(pq));
			if (startup == TRUE)
				exit(EXIT_FAILURE);
			wallops("\2DATABASE ERROR\2: %s", PQerrorMessage(pq));
		}
	}
}

/*
 * Returns a PGresult on success.
 * Returns NULL on failure, and logs the error if debug is enabled.
 */
static PGresult *safe_query(const char *string, ...)
{
	va_list args;
	char buf[BUFSIZE * 3];
	PGresult *res;

	if (db_errored)
		return NULL;

	va_start(args, string);
	vsnprintf(buf, BUFSIZE * 3, string, args);
	va_end(args);

	slog(LG_DEBUG, "executing: %s", buf);

	res = PQexec(pq, buf);

	if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slog(LG_ERROR, "There was an error executing the query:");
		slog(LG_ERROR, "       query error: %s", PQresultErrorMessage(res));
		slog(LG_ERROR, "    database error: %s", PQerrorMessage(pq));

		wallops("\2DATABASE ERROR\2: query error: %s", PQresultErrorMessage(res));
		wallops("\2DATABASE ERROR\2: database error: %s", PQerrorMessage(pq));

		db_errored = 1;
		return NULL;
	}

	return res;
}

/*
 * Writes a clean snapshot of Atheme's state to the SQL database.
 *
 * This uses transactions, so writes are safe.
 */
static void postgresql_db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	node_t *n, *tn, *tn2;
	uint32_t i, ii, iii;
	PGresult *res;

	slog(LG_DEBUG, "db_save(): saving myusers");

	ii = 0;

	db_connect(FALSE);

	/* safety */
	safe_query("BEGIN TRANSACTION;");

	/* clear everything out. */
	safe_query("DELETE FROM ACCOUNTS;");
	safe_query("DELETE FROM ACCOUNT_METADATA;");
	safe_query("DELETE FROM ACCOUNT_MEMOS;");
	safe_query("DELETE FROM ACCOUNT_MEMO_IGNORES;");
	safe_query("DELETE FROM CHANNELS;");
	safe_query("DELETE FROM CHANNEL_METADATA;");
	safe_query("DELETE FROM CHANNEL_ACCESS;");
	safe_query("DELETE FROM KLINES;");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			char user[BUFSIZE];
			char pass[BUFSIZE];
			char email[BUFSIZE];

			mu = (myuser_t *)n->data;

			PQescapeString(user, mu->name, BUFSIZE);
			PQescapeString(pass, mu->pass, BUFSIZE);
			PQescapeString(email, mu->email, BUFSIZE);

			res = safe_query("INSERT INTO ACCOUNTS(ID, USERNAME, PASSWORD, EMAIL, REGISTERED, LASTLOGIN, FLAGS) "
					"VALUES (%d, '%s', '%s', '%s', %ld, %ld, %ld)", ii, user, pass, email,
					(long) mu->registered, (long) mu->lastlogin, (long) mu->flags);

			LIST_FOREACH(tn, mu->metadata.head)
			{
				char key[BUFSIZE], keyval[BUFSIZE];
				metadata_t *md = (metadata_t *)tn->data;

				PQescapeString(key, md->name, BUFSIZE);
				PQescapeString(keyval, md->value, BUFSIZE);

				res = safe_query("INSERT INTO ACCOUNT_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s');", ii, key, keyval);
			}

			LIST_FOREACH(tn, mu->memos.head)
			{
				char sender[BUFSIZE], text[BUFSIZE];
				mymemo_t *mz = (mymemo_t *)tn->data;

				PQescapeString(sender, mz->sender, BUFSIZE);
				PQescapeString(text, mz->text, BUFSIZE);

				res = safe_query("INSERT INTO ACCOUNT_MEMOS(ID, PARENT, SENDER, TIME, STATUS, TEXT) VALUES ("
						"DEFAULT, %d, '%s', %ld, %ld, '%s');", ii, sender, mz->sent, mz->status, text);
			}
			
			LIST_FOREACH(tn, mu->memo_ignores.head)
			{
				char target[BUFSIZE], *temp = (char *)tn->data;
				PQescapeString(target, temp, strlen(temp));
				
				res = safe_query("INSERT INTO ACCOUNT_MEMO_IGNORES(ID, PARENT, TARGET) VALUES(DEFAULT, %d, '%s')", ii, target);
						
				free(target);
			}

			ii++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving mychans");

	ii = 0;
	iii = 0;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			char cname[BUFSIZE], cfounder[BUFSIZE];
			char ckey[BUFSIZE];

			mc = (mychan_t *)n->data;

			PQescapeString(cname, mc->name, BUFSIZE);
			/* founder shouldn't contain anything bad, but be safe anyway */
			PQescapeString(cfounder, mc->founder->name, BUFSIZE);

			if (mc->mlock_key)
				PQescapeString(ckey, mc->mlock_key, BUFSIZE);

			res = safe_query("INSERT INTO CHANNELS(ID, NAME, FOUNDER, REGISTERED, LASTUSED, "
					"FLAGS, MLOCK_ON, MLOCK_OFF, MLOCK_LIMIT, MLOCK_KEY) VALUES ("
					"%d, '%s', '%s', %ld, %ld, %ld, %ld, %ld, %ld, '%s');", ii,
					cname, cfounder, (long)mc->registered, (long)mc->used, (long)mc->flags,
					(long)mc->mlock_on, (long)mc->mlock_off, (long)mc->mlock_limit,
					mc->mlock_key ? ckey : "");

			LIST_FOREACH(tn, mc->chanacs.head)
			{
				char caaccount[BUFSIZE];

				ca = (chanacs_t *)tn->data;

				if (!ca->myuser)
					PQescapeString(caaccount, ca->host, BUFSIZE);
				else
					PQescapeString(caaccount, ca->myuser->name, BUFSIZE);

				res = safe_query("INSERT INTO CHANNEL_ACCESS(ID, PARENT, ACCOUNT, PERMISSIONS) VALUES ("
						"%d, %d, '%s', '%s');", iii, ii, caaccount,
						bitmask_to_flags(ca->level, chanacs_flags));

				LIST_FOREACH(tn2, ca->metadata.head)
				{
					char key[BUFSIZE], keyval[BUFSIZE];
					metadata_t *md = (metadata_t *)tn2->data;

					PQescapeString(key, md->name, BUFSIZE);
					PQescapeString(keyval, md->name, BUFSIZE);

					res = safe_query("INSERT INTO CHANNEL_ACCESS_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES("
						"DEFAULT, %d, '%s', '%s');", iii, key, keyval);
				}

				iii++;
			}

			LIST_FOREACH(tn, mc->metadata.head)
			{
				char key[BUFSIZE], keyval[BUFSIZE];
				metadata_t *md = (metadata_t *)tn->data;

				PQescapeString(key, md->name, BUFSIZE);
				PQescapeString(keyval, md->value, BUFSIZE);

				res = safe_query("INSERT INTO CHANNEL_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s');", ii, key, keyval);
			}

			ii++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving klines");

	ii = 0;

	LIST_FOREACH(n, klnlist.head)
	{
		char kuser[BUFSIZE], khost[BUFSIZE];
		char ksetby[BUFSIZE], kreason[BUFSIZE];
		metadata_t *md = (metadata_t *)tn->data;

		k = (kline_t *)n->data;

		PQescapeString(kuser, k->user, BUFSIZE);
		PQescapeString(khost, k->host, BUFSIZE);
		PQescapeString(ksetby, k->setby, BUFSIZE);
		PQescapeString(kreason, k->reason, BUFSIZE);

		res = safe_query("INSERT INTO KLINES(ID, USERNAME, HOSTNAME, DURATION, SETTIME, SETTER, REASON) VALUES ("
				"%d, '%s', '%s', %ld, %ld, '%s', '%s');", ii, kuser, khost, k->duration, (long) k->settime,
				ksetby, kreason);

		ii++;
	}

	/* done, commit */
	safe_query("COMMIT TRANSACTION;");
}

/* loads atheme.db */
static void postgresql_db_load(void)
{
	myuser_t *mu;
	chanacs_t *ca;
	node_t *n;
	mychan_t *mc;
	kline_t *k;
	uint32_t i = 0, muin = 0, mcin = 0, kin = 0;
	PGresult *res;

	db_connect(TRUE);

	res = safe_query("SELECT ID, USERNAME, PASSWORD, EMAIL, REGISTERED, LASTLOGIN, "
				"FLAGS FROM ACCOUNTS;");
	muin = PQntuples(res);

	slog(LG_DEBUG, "db_load(): Got %d myusers from SQL.", muin);

	for (i = 0; i < muin; i++)
	{
		char *muname, *mupass, *muemail;
		uint32_t uid, umd, ii;
		PGresult *res2;

		uid = atoi(PQgetvalue(res, i, 0));
		muname = PQgetvalue(res, i, 1);
		mupass = PQgetvalue(res, i, 2);
		muemail = PQgetvalue(res, i, 3);

		mu = myuser_add(muname, mupass, muemail, atoi(PQgetvalue(res, i, 6)));

		mu->registered = atoi(PQgetvalue(res, i, 4));
		mu->lastlogin = atoi(PQgetvalue(res, i, 5));

		res2 = safe_query("SELECT ID, PARENT, KEYNAME, VALUE FROM ACCOUNT_METADATA WHERE PARENT=%d;", uid);
		umd = PQntuples(res2);

		for (ii = 0; ii < umd; ii++)
		{
			char *key = PQgetvalue(res2, ii, 2);
			char *keyval = PQgetvalue(res2, ii, 3);

			if (!key || !keyval)
			{
				slog(LG_DEBUG, "db_load(): ignoring invalid user metadata entry.");
				continue;
			}

			metadata_add(mu, METADATA_USER, key, keyval);
		}

		/* Memos */
		
		PQclear(res2);

		res2 = safe_query("SELECT ID, PARENT, SENDER, TIME, STATUS, TEXT FROM ACCOUNT_MEMOS WHERE PARENT=%d;", uid);
		umd = PQntuples(res2);

		for (ii = 0; ii < umd; ii++)
		{
                        char *sender, *text;
                        time_t mtime; 
                        uint32_t status;
                        mymemo_t *mz;

			sender = PQgetvalue(res2, ii, 2);
			mtime = atoi(PQgetvalue(res2, ii, 3));
			status = atoi(PQgetvalue(res2, ii, 4));
                        text = PQgetvalue(res2, ii, 5);

                        if (!mu)
                        {
                                slog(LG_DEBUG, "db_load(): WTF -- memo for unknown account");
                                continue;
                        }

                        if (!sender || !mtime || !text)
                                continue;

                        mz = smalloc(sizeof(mymemo_t));

                        strlcpy(mz->sender, sender, NICKLEN);
                        strlcpy(mz->text, text, MEMOLEN);
                        mz->sent = mtime;
                        mz->status = status;

			if (mz->status == MEMO_NEW)
				mu->memoct_new++;

                        node_add(mz, node_create(), &mu->memos);
		}
		
		/* MemoServ ignores */
		
		PQclear(res2);

		res2 = safe_query("SELECT * FROM ACCOUNT_MEMO_IGNORES WHERE PARENT=%d;", uid);
		umd = PQntuples(res2);

		for (ii = 0; ii < umd; ii++)
		{
			char *target;
			char *strbuf;

			target = PQgetvalue(res2, ii, 2);
			
			if (!mu)
			{
				slog(LG_DEBUG, "db_load(): invalid ignore");
				continue;
			}
			
			if (!target)
				continue;
			
			strbuf = smalloc(sizeof(char[NICKLEN]));
			strlcpy(strbuf,target,NICKLEN-1);
			
			node_add(strbuf, node_create(), &mu->memo_ignores);
		}
		
		PQclear(res2);
	}

	PQclear(res);
	res = safe_query("SELECT ID, NAME, FOUNDER, REGISTERED, LASTUSED, FLAGS, MLOCK_ON, "
				"MLOCK_OFF, MLOCK_LIMIT, MLOCK_KEY FROM CHANNELS;");
	mcin = PQntuples(res);

	for (i = 0; i < mcin; i++)
	{
		char *mcname;
		uint32_t cain, mdin, ii;
		PGresult *res2;

		mcname = PQgetvalue(res, i, 1);

		mc = mychan_add(mcname);

		mc->founder = myuser_find(PQgetvalue(res, i, 2));

		if (!mc->founder)
		{
			slog(LG_DEBUG, "db_load(): channel %s has no founder, dropping.", mc->name);
			mychan_delete(mc->name);
			continue;
		}

		mc->registered = atoi(PQgetvalue(res, i, 3));
		mc->used = atoi(PQgetvalue(res, i, 4));
		mc->flags = atoi(PQgetvalue(res, i, 5));

		mc->mlock_on = atoi(PQgetvalue(res, i, 6));
		mc->mlock_off = atoi(PQgetvalue(res, i, 7));
		mc->mlock_limit = atoi(PQgetvalue(res, i, 8));
		mc->mlock_key = sstrdup(PQgetvalue(res, i, 9));

		/* SELECT * FROM CHANNEL_ACCESS WHERE PARENT=21 */
		res2 = safe_query("SELECT ID, PARENT, ACCOUNT, PERMISSIONS "
					"FROM CHANNEL_ACCESS WHERE PARENT=%s;", PQgetvalue(res, i, 0));
		cain = PQntuples(res2);

		for (ii = 0; ii < cain; ii++)
		{
			char *username, *permissions;
			PGresult *res3;
			uint32_t fl, ca_mdin, iii;

			username = PQgetvalue(res2, ii, 2);
			permissions = PQgetvalue(res2, ii, 3);
			fl = flags_to_bitmask(permissions, chanacs_flags, 0x0);

			mu = myuser_find(username);

			if (!mu)
				ca = chanacs_add_host(mc, username, fl);
			else
				ca = chanacs_add(mc, mu, fl);

			res3 = safe_query("SELECT ID, PARENT, KEYNAME, VALUE "
					"FROM CHANNEL_ACCESS_METADATA WHERE PARENT=%s;", PQgetvalue(res, i, 0));

			ca_mdin = PQntuples(res3);

			for (iii = 0; iii < ca_mdin; iii++)
			{
				char *key = PQgetvalue(res3, iii, 2);
				char *keyval = PQgetvalue(res3, iii, 3);

				if (!key || !keyval)
				{
					slog(LG_DEBUG, "db_load(): ignoring invalid channel access metadata entry.");
					continue;
				}

				metadata_add(ca, METADATA_CHANACS, key, keyval);
			}

			PQclear(res3);
		}

		PQclear(res2);

		/* SELECT * FROM CHANNEL_METADATA WHERE PARENT=21 */
		res2 = safe_query("SELECT ID, PARENT, KEYNAME, VALUE "
					"FROM CHANNEL_METADATA WHERE PARENT=%s;", PQgetvalue(res, i, 0));
		mdin = PQntuples(res2);

		for (ii = 0; ii < mdin; ii++)
		{
			char *key = PQgetvalue(res2, ii, 2);
			char *keyval = PQgetvalue(res2, ii, 3);

			if (!key || !keyval)
			{
				slog(LG_DEBUG, "db_load(): ignoring invalid channel metadata entry.");
				continue;
			}

			metadata_add(mc, METADATA_CHANNEL, key, keyval);
		}

		PQclear(res2);
	}

	PQclear(res);

	res = safe_query("SELECT ID, USERNAME, HOSTNAME, DURATION, "
				"SETTIME, SETTER, REASON FROM KLINES;");
	kin = PQntuples(res);

	for (i = 0; i < kin; i++)
	{
		char *user, *host, *reason, *setby;
		uint32_t duration, settime;

		user = PQgetvalue(res, i, 1);
		host = PQgetvalue(res, i, 2);
		setby = PQgetvalue(res, i, 5);
		reason = PQgetvalue(res, i, 6);

		duration = atoi(PQgetvalue(res, i, 3));
		settime = atoi(PQgetvalue(res, i, 4));

		k = kline_add(user, host, reason, duration);
		k->settime = settime;
		/* XXX this is not nice, oh well -- jilles */
		k->expires = k->settime + k->duration;
		k->setby = sstrdup(setby);
	}

	PQclear(res);
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &postgresql_db_load;
	db_save = &postgresql_db_save;

	backend_loaded = TRUE;

}
