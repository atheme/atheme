/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the database
 * using MySQL.
 *
 * $Id: mysql.c 3639 2005-11-07 22:57:22Z alambert $
 */

#include "atheme.h"
#include <mysql.h>

DECLARE_MODULE_V1
(
	"backend/mysql", TRUE, _modinit, NULL,
	"$Id: mysql.c 3639 2005-11-07 22:57:22Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

#define SQL_BUFSIZE (BUFSIZE * 3)

static MYSQL     *mysql = NULL;
static int       db_errored = 0;

static void db_connect(boolean_t startup)
{
	char dbcredentials[SQL_BUFSIZE];
	uint32_t dbo_port;
	int32_t retval;

	db_errored = 0;

	/* Open the db connection up. */
	mysql = mysql_init(NULL);

	if (!database_options.port)
		dbo_port = mysql->port;
	else
		dbo_port = database_options.port;

	snprintf(dbcredentials, BUFSIZE, "host=%s dbname=%s user=%s password=%s port=%d",
			database_options.host, database_options.database, database_options.user, database_options.pass,
			dbo_port);

	slog(LG_DEBUG, "Connecting to MySQL provider using these credentials:");
	slog(LG_DEBUG, "      %s", dbcredentials); 

	if (mysql_real_connect(mysql, database_options.host, database_options.user, database_options.pass, NULL, mysql->port, 0, 0)==NULL)
	{
		slog(LG_ERROR, "There was an error connecting to the database:");
		slog(LG_ERROR, "    %s", mysql_error(mysql));

		if (startup == TRUE)
			exit(1);
	}

	if((retval = mysql_select_db(mysql, database_options.database)))
	{
		slog(LG_ERROR, "There was an error selecting the database:");
		slog(LG_ERROR, "    %s", mysql_error(mysql));

		if (startup == TRUE)
			exit(1);
	}
}

/*
 * Returns a MYSQL_RES on success.
 * Returns NULL on failure, and logs the error if debug is enabled.
 */
static MYSQL_RES *safe_query(const char *string, ...)
{
	va_list args;
	char buf[SQL_BUFSIZE];
	MYSQL_RES *res = NULL;

	if (db_errored)
		return NULL;

	va_start(args, string);
	vsnprintf(buf, SQL_BUFSIZE, string, args);
	va_end(args);

	slog(LG_DEBUG, "executing: %s", buf);

	mysql_ping(mysql);

	if (mysql_query(mysql, buf))
	{
		slog(LG_ERROR, "There was an error executing the query:");
		slog(LG_ERROR, "    query error: %s", mysql_error(mysql));

		wallops("\2DATABASE ERROR\2: database error: %s", mysql_error(mysql));
		db_errored = 1;

		return NULL;
	}

	if ((res = mysql_store_result(mysql)) == NULL)
	{
		if(mysql_field_count(mysql) == 0)
		{
			return NULL;
		}
		else
		{
			slog(LG_ERROR, "There was an error executing the query:");
			slog(LG_ERROR, "    query error: %s", mysql_error(mysql));

			wallops("\2DATABASE ERROR\2: database error: %s", mysql_error(mysql));
			db_errored = 1;

			return NULL;
		}
	}

	return res;
}

/*
 * Returns the length of 'str' as escaped by mysql_real_escape_string().
 * Returns 0 (zero) on error.
 *
 * NOTE: This function will smalloc() memory to hold the escaped string,
 * and must be free()'d when no-longer needed.
 *
 * Rewritten 20 hours before the 0.3 tree was set to freeze by nenolod.
 */
static char *escape_string(char *str)
{
	char *buf;
	uint32_t str_len;

	if (!str)
	{
		slog(LG_DEBUG, "escape_string(): no string given to escape!");
		return NULL;
	}

	str_len = strlen(str);

	buf = smalloc((str_len * 2) + 1);
	memset(buf, 0, (str_len * 2) + 1);

	mysql_real_escape_string(mysql, buf, str, str_len);

	return buf;
}

/*
 * Writes a clean snapshot of Atheme's state to the SQL database.
 */
static void mysql_db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	node_t *n, *tn, *tn2;
	int retval;
	uint32_t i, ii, iii;
	MYSQL_RES *res = NULL;

	db_connect(FALSE);

	slog(LG_DEBUG, "db_save(): saving myusers");

	ii = 0;

	/* safety */
	safe_query("BEGIN");

	/* clear everything out. */
	safe_query("DELETE FROM ACCOUNTS");
	safe_query("DELETE FROM ACCOUNT_METADATA");
	safe_query("DELETE FROM ACCOUNT_MEMOS");
	safe_query("DELETE FROM ACCOUNT_MEMO_IGNORES");
	safe_query("DELETE FROM CHANNELS");
	safe_query("DELETE FROM CHANNEL_METADATA");
	safe_query("DELETE FROM CHANNEL_ACCESS");
	safe_query("DELETE FROM KLINES");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			char *user, *pass, *email;

			mu = (myuser_t *)n->data;

			user = escape_string(mu->name);
			pass = escape_string(mu->pass);
			email = escape_string(mu->email);

			res = safe_query("INSERT INTO ACCOUNTS(ID, USERNAME, PASSWORD, EMAIL, REGISTERED, LASTLOGIN, FLAGS) "
					"VALUES (%d, '%s', '%s', '%s', %ld, %ld, %ld)", ii, user, pass, email,
					(long) mu->registered, (long) mu->lastlogin, (long) mu->flags);

			free(user);
			free(pass);
			free(email);

			LIST_FOREACH(tn, mu->metadata.head)
			{
				char *key, *keyval;
				metadata_t *md = (metadata_t *)tn->data;

				key = escape_string(md->name);
				keyval = escape_string(md->value);

				res = safe_query("INSERT INTO ACCOUNT_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s')", ii, key, keyval);

				free(key);
				free(keyval);
			}

			LIST_FOREACH(tn, mu->memos.head)
			{
				char *sender, *text;
				mymemo_t *md = (mymemo_t *)tn->data;

				sender = escape_string(md->sender);
				text = escape_string(md->text);

				res = safe_query("INSERT INTO ACCOUNT_MEMOS(ID, PARENT, SENDER, TIME, STATUS, TEXT) VALUES ("
						"DEFAULT, %d, '%s', %ld, %ld, '%s')", ii, sender, md->sent, md->status, text);

				free(sender);
				free(text);
			}
			
			LIST_FOREACH(tn, mu->memo_ignores.head)
			{
				char *target, *temp = (char *)tn->data;
				target = escape_string(temp);
				
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
			char *cname, *cfounder;
			char *ckey = NULL;

			mc = (mychan_t *)n->data;

			cname = escape_string(mc->name);
			cfounder = escape_string(mc->founder->name);

			if (mc->mlock_key)
				ckey = escape_string(mc->mlock_key);

			res = safe_query("INSERT INTO CHANNELS(ID, NAME, FOUNDER, REGISTERED, LASTUSED, "
					"FLAGS, MLOCK_ON, MLOCK_OFF, MLOCK_LIMIT, MLOCK_KEY) VALUES ("
					"%d, '%s', '%s', %ld, %ld, %ld, %ld, %ld, %ld, '%s')", ii,
					cname, cfounder, (long)mc->registered, (long)mc->used, (long)mc->flags,
					(long)mc->mlock_on, (long)mc->mlock_off, (long)mc->mlock_limit,
					mc->mlock_key ? ckey : "");

			free(cname);
			free(cfounder);
			if (ckey)
				free(ckey);

			LIST_FOREACH(tn, mc->chanacs.head)
			{
				char *caaccount;

				ca = (chanacs_t *)tn->data;

				if (!ca->myuser)
					caaccount = escape_string(ca->host);
				else
					caaccount = escape_string(ca->myuser->name);

				res = safe_query("INSERT INTO CHANNEL_ACCESS(ID, PARENT, ACCOUNT, PERMISSIONS) VALUES ("
						"%d, %d, '%s', '%s')", iii, ii, caaccount,
						bitmask_to_flags(ca->level, chanacs_flags));

				free(caaccount);

				LIST_FOREACH(tn2, ca->metadata.head)
				{
					char *key, *keyval;
					metadata_t *md = (metadata_t *)tn2->data;

					key = escape_string(md->name);
					keyval = escape_string(md->value);

					res = safe_query("INSERT INTO CHANNEL_ACCESS_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES("
						"DEFAULT, %d, '%s', '%s')", iii, key, keyval);

					free(key);
					free(keyval);
				}

				iii++;
			}

			LIST_FOREACH(tn, mc->metadata.head)
			{
				char *key, *keyval;
				metadata_t *md = (metadata_t *)tn->data;

				key = escape_string(md->name);
				keyval = escape_string(md->value);

				res = safe_query("INSERT INTO CHANNEL_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s')", ii, key, keyval);

				free(key);
				free(keyval);
			}

			ii++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving klines");

	ii = 0;

	LIST_FOREACH(n, klnlist.head)
	{
		char *kuser, *khost;
		char *ksetby, *kreason;
		k = (kline_t *)n->data;

		kuser = escape_string(k->user);
		khost = escape_string(k->host);
		ksetby = escape_string(k->setby);
		kreason = escape_string(k->reason);

		res = safe_query("INSERT INTO KLINES(ID, USERNAME, HOSTNAME, DURATION, SETTIME, SETTER, REASON) VALUES ("
				"%d, '%s', '%s', %ld, %ld, '%s', '%s')", ii, kuser, khost, k->duration, (long) k->settime,
				ksetby, kreason);

		free(kuser);
		free(khost);
		free(ksetby);
		free(kreason);

		ii++;
	}

	/* done, commit */
	safe_query("COMMIT");

	/* close the connection, we cannot persist like with postgre. */
	mysql_close(mysql);
}

/* loads atheme.db */
static void mysql_db_load(void)
{
	myuser_t *mu;
	chanacs_t *ca;
	node_t *n;
	mychan_t *mc;
	kline_t *k;
	int retval;
	uint32_t i = 0, muin = 0, mcin = 0, kin = 0;
	uint32_t dbo_port;
	char dbcredentials[BUFSIZE];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;

	db_connect(TRUE);

	res = safe_query("SELECT ID, USERNAME, PASSWORD, EMAIL, REGISTERED, LASTLOGIN, "
				"FLAGS FROM ACCOUNTS");
	muin = mysql_num_rows(res);

	slog(LG_DEBUG, "db_load(): Got %d myusers from SQL.", muin);

	while ((row = mysql_fetch_row(res)))
	{
		uint32_t uid, umd;
		MYSQL_RES *res2 = NULL;
		MYSQL_ROW row2 = NULL;

		uid = atoi(row[0]);
		mu = myuser_add(row[1], row[2], row[3]);

		mu->registered = atoi(row[4]);
		mu->lastlogin = atoi(row[5]);
		mu->flags = atoi(row[6]);

		res2 = safe_query("SELECT ID, PARENT, KEYNAME, VALUE FROM ACCOUNT_METADATA WHERE PARENT=%d", uid);
		umd = mysql_num_rows(res2);

		while ((row2 = mysql_fetch_row(res2)))
			metadata_add(mu, METADATA_USER, row2[2], row2[3]);

		mysql_free_result(res2);

		/* Memos */
		
		res2 = safe_query("SELECT ID, PARENT, SENDER, TIME, STATUS, TEXT FROM ACCOUNT_MEMOS WHERE PARENT=%d", uid);
		umd = mysql_num_rows(res2);

		while ((row2 = mysql_fetch_row(res2)))
		{
                        char *sender = row2[2];
                        time_t time = atoi(row2[3]);
                        uint32_t status = atoi(row2[4]);
                        char *text = row2[5];
                        mymemo_t *mz;

                        if (!mu)
                        {
                                slog(LG_DEBUG, "db_load(): WTF -- memo for unknown account");
                                continue;
                        }

                        if (!sender || !time || !text)
                                continue;

                        mz = smalloc(sizeof(mymemo_t));

                        strlcpy(mz->sender, sender, NICKLEN);
                        strlcpy(mz->text, text, MEMOLEN);
                        mz->sent = time;
                        mz->status = status;

			if (mz->status == MEMO_NEW)
				mu->memoct_new++;

                        node_add(mz, node_create(), &mu->memos);
		}
		
		mysql_free_result(res2);
		
		/* MemoServ ignores */
		
		res2 = safe_query("SELECT * FROM ACCOUNT_MEMO_IGNORES WHERE PARENT=%d", uid);
		umd = mysql_num_rows(res2);
		
		while ((row2 = mysql_fetch_row(res2)))
		{
			char *target = row2[2];
			char *strbuf;
			
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
		
		mysql_free_result(res2);
	}

	mysql_free_result(res);
	res = NULL;

	res = safe_query("SELECT ID, NAME, FOUNDER, REGISTERED, LASTUSED, FLAGS, MLOCK_ON, "
				"MLOCK_OFF, MLOCK_LIMIT, MLOCK_KEY FROM CHANNELS");
	mcin = mysql_num_rows(res);

	while ((row = mysql_fetch_row(res)))
	{
		char *mcname;
		uint32_t cain, mdin, ii;
		MYSQL_RES *res2 = NULL;
		MYSQL_ROW row2 = NULL;

		mc = mychan_add(row[1]);

		mc->founder = myuser_find(row[2]);

		if (!mc->founder)
		{
			slog(LG_DEBUG, "db_load(): channel %s has no founder, dropping.", mc->name);
			mychan_delete(mc->name);
			continue;
		}

		mc->registered = atoi(row[3]);
		mc->used = atoi(row[4]);
		mc->flags = atoi(row[5]);

		mc->mlock_on = atoi(row[6]);
		mc->mlock_off = atoi(row[7]);
		mc->mlock_limit = atoi(row[8]);
		mc->mlock_key = sstrdup(row[9]);

		/* SELECT * FROM CHANNEL_ACCESS WHERE PARENT=21 */
		res2 = safe_query("SELECT ID, PARENT, ACCOUNT, PERMISSIONS FROM CHANNEL_ACCESS WHERE PARENT='%s'", row[0]);

		cain = mysql_num_rows(res2);

		while ((row2 = mysql_fetch_row(res2)))
		{
			char *username, *permissions;
			MYSQL_RES *res3 = NULL;
			MYSQL_ROW row3 = NULL;
			uint32_t fl, ca_mdin, iii;

			mu = myuser_find(row2[2]);

			fl = flags_to_bitmask(row2[3], chanacs_flags, 0x0);

			if (!mu)
				ca = chanacs_add_host(mc, row2[2], fl);
			else
				ca = chanacs_add(mc, mu, fl);

			res3 = safe_query("SELECT ID, PARENT, KEYNAME, VALUE FROM CHANNEL_ACCESS_METADATA WHERE PARENT='%s'", row2[0]);

			ca_mdin = mysql_num_rows(res3);
			while ((row3 = mysql_fetch_row(res3)))
				metadata_add(ca, METADATA_CHANACS, row3[2], row3[3]);

			mysql_free_result(res3);
			res3 = NULL;
		}
		mysql_free_result(res2);
		res2 = NULL;

		/* SELECT * FROM CHANNEL_METADATA WHERE PARENT=21 */
		res2 = safe_query("SELECT ID, PARENT, KEYNAME, VALUE FROM CHANNEL_METADATA WHERE PARENT='%s'", row[0]);
		mdin = mysql_num_rows(res2);

		while ((row2 = mysql_fetch_row(res2)))
			metadata_add(mc, METADATA_CHANNEL, row[2], row[3]);

		mysql_free_result(res2);
		res2 = NULL;
	}

	mysql_free_result(res);
	res = NULL;

	res = safe_query("SELECT ID, USERNAME, HOSTNAME, DURATION, SETTIME, SETTER, REASON FROM KLINES");
	kin = mysql_num_rows(res);

	while ((row = mysql_fetch_row(res)))
	{
		uint32_t duration, settime;

		duration = atoi(row[3]);
		settime = atoi(row[4]);
		k = kline_add(row[1], row[2], row[6], duration);
		k->settime = settime;
		k->setby = sstrdup(row[5]);
	}

	mysql_free_result(res);
	res = NULL;

	/* close the connection, we cannot persist like with postgre. */
	mysql_close(mysql);
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &mysql_db_load;
	db_save = &mysql_db_save;

	backend_loaded = TRUE;

	/* for compatibility --nenolod */
	mysql_server_init(0, NULL, NULL);
}
