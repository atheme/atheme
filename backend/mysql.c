/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the database
 * using MySQL.
 *
 * $Id: mysql.c 2787 2005-10-09 00:27:33Z nenolod $
 */

#include "atheme.h"
#include <mysql.h>

DECLARE_MODULE_V1
(
	"backend/mysql", TRUE, _modinit, NULL,
	"$Id: mysql.c 2787 2005-10-09 00:27:33Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

#define SQL_BUFSIZE (BUFSIZE * 3)

static MYSQL     *mysql = NULL;

/*
 * Returns a MYSQL_RES on success.
 * Returns NULL on failure, and logs the error if debug is enabled.
 */
static MYSQL_RES *safe_query(const char *string, ...)
{
	va_list args;
	char buf[SQL_BUFSIZE];
	MYSQL_RES *res = NULL;

	va_start(args, string);
	vsnprintf(buf, SQL_BUFSIZE, string, args);
	va_end(args);

	slog(LG_DEBUG, "executing: %s", buf);

	if (mysql_real_query(mysql, buf, sizeof(buf)))
	{
		slog(LG_DEBUG, "There was an error executing the query:");
		slog(LG_DEBUG, "    query error: %s", mysql_error(mysql));

		wallops("\2DATABASE ERROR\2: database error: %s", mysql_error(mysql));

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
			slog(LG_DEBUG, "There was an error executing the query:");
			slog(LG_DEBUG, "    query error: %s", mysql_error(mysql));

			wallops("\2DATABASE ERROR\2: database error: %s", mysql_error(mysql));

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
 */
static uint32_t escape_string(char **buf, char *str, uint32_t str_len)
{
	if (!buf || !*buf) {
		slog(LG_DEBUG, "buf argument escape_string() is NULL ! o.0");
		return 0;
	}
	if (!str || !*str) {
		slog(LG_DEBUG, "str argument to escape_string() is NULL or of zero-length ! o.0");
		return 0;
	}
	if (!str_len) {
		slog(LG_DEBUG, "str_len argument to escape_string() is zero ! o.0");
		return 0;
	}

	*buf = smalloc((str_len * 2) + 1);
	memset(*buf, 0, (str_len * 2) + 1);

	return mysql_real_escape_string(mysql, *buf, str, str_len);
}

/*
 * Writes a clean snapshot of Atheme's state to the SQL database.
 *
 * This uses transactions, so writes are safe.
 */
static void mysql_db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	node_t *n, *tn, *tn2;
	int retval;
	uint32_t i, ii, iii, dbo_port;
	MYSQL_RES *res = NULL;

	slog(LG_DEBUG, "db_save(): saving myusers");

	ii = 0;

	/* insert connect */
	mysql = mysql_init(NULL);
	if (!mysql) {
		/* might not get there, but hey, it's worth a try (: */
		slog(LG_DEBUG, "mysql_init() failed (out of memory condition)");
		exit(1);
	}

	if (!database_options.port)
		dbo_port = mysql->port;
	else
		dbo_port = database_options.port;

	if (mysql_real_connect(mysql, database_options.host, database_options.user,
	                       database_options.pass, NULL, dbo_port, 0, 0) == NULL)
	{
		slog(LG_DEBUG, "There was an error connecting to the database:");
		slog(LG_DEBUG, "    %s", mysql_error(mysql));

		wallops("\2DATABASE ERROR\2: connection error: %s", mysql_error(mysql));
		exit(1);
	}

	if((retval = mysql_select_db(mysql, database_options.database)))
	{
		slog(LG_DEBUG, "There was an error selecting the database:");
		slog(LG_DEBUG, "    %s", mysql_error(mysql));

		wallops("\2DATABASE ERROR\2: database selection error: %s", mysql_error(mysql));
		exit(1);
	}

	/* safety */
	/*safe_query("BEGIN");*/

	/* clear everything out. */
	safe_query("TRUNCATE TABLE ACCOUNTS");
	safe_query("TRUNCATE TABLE ACCOUNT_METADATA");
	safe_query("TRUNCATE TABLE CHANNELS");
	safe_query("TRUNCATE TABLE CHANNEL_METADATA");
	safe_query("TRUNCATE TABLE CHANNEL_ACCESS");
	safe_query("TRUNCATE TABLE KLINES");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			char *user, *pass, *email;

			mu = (myuser_t *)n->data;

			escape_string(&user, mu->name, strlen(mu->name));
			escape_string(&pass, mu->pass, strlen(mu->pass));
			escape_string(&email, mu->email, strlen(mu->email));

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

				escape_string(&key, md->name, strlen(md->name));
				escape_string(&keyval, md->value, strlen(md->value));

				res = safe_query("INSERT INTO ACCOUNT_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s')", ii, key, keyval);

				free(key);
				free(keyval);
			}

			LIST_FOREACH(tn, mu->memos.head)
			{
				char *sender, *text;
				mymemo_t *md = (mymemo_t *)tn->data;

				escape_string(&sender, md->sender, strlen(md->sender));
				escape_string(&text, md->text, strlen(md->text));

				res = safe_query("INSERT INTO ACCOUNT_MEMOS(ID, PARENT, SENDER, TIME, STATUS, TEXT) VALUES ("
						"DEFAULT, %d, '%s', %ld, %ld, '%s')", ii, sender, md->sent, md->status, text);

				free(sender);
				free(text);
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

			escape_string(&cname, mc->name, strlen(mc->name));
			/* founder shouldn't contain anything bad, but be safe anyway */
			escape_string(&cfounder, mc->founder->name, strlen(mc->founder->name));

			if (mc->mlock_key)
				escape_string(&ckey, mc->mlock_key, strlen(mc->mlock_key));

			/* OK to clear out legacy "ENTRYMSG" column; it's saved in metadata.
			 * right now we don't have versioning for postgres. eventually we can
			 * drop the "ENTRYMSG" column. not too concerned about backwards
			 * compatibility since this backend is still experimental.  --alambert
			 *
			 * ditto for "URL" column and failure logging columns in "ACCOUNTS".
			 */
			res = safe_query("INSERT INTO CHANNELS(ID, NAME, FOUNDER, REGISTERED, LASTUSED, "
					"FLAGS, MLOCK_ON, MLOCK_OFF, MLOCK_LIMIT, MLOCK_KEY, URL, ENTRYMSG) VALUES ("
					"%d, '%s', '%s', %ld, %ld, %ld, %ld, %ld, %ld, '%s', '', '')", ii,
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
					escape_string(&caaccount, ca->host, strlen(ca->host));
				else
					escape_string(&caaccount, ca->myuser->name, strlen(ca->myuser->name));

				res = safe_query("INSERT INTO CHANNEL_ACCESS(ID, PARENT, ACCOUNT, PERMISSIONS) VALUES ("
						"%d, %d, '%s', '%s')", iii, ii, caaccount,
						bitmask_to_flags(ca->level, chanacs_flags));

				free(caaccount);

				LIST_FOREACH(tn2, ca->metadata.head)
				{
					char *key, *keyval;
					metadata_t *md = (metadata_t *)tn2->data;

					escape_string(&key, md->name, strlen(md->name));
					escape_string(&keyval, md->value, strlen(md->value));

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

				escape_string(&key, md->name, strlen(md->name));
				escape_string(&keyval, md->value, strlen(md->value));

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
		metadata_t *md = (metadata_t *)tn->data;

		k = (kline_t *)n->data;

		escape_string(&kuser, k->user, strlen(k->user));
		escape_string(&khost, k->host, strlen(k->host));
		escape_string(&ksetby, k->setby, strlen(k->setby));
		escape_string(&kreason, k->reason, strlen(k->reason));

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
	/*safe_query("COMMIT");*/
}

/* loads atheme.db */
static void mysql_db_load(void)
{
	myuser_t *mu;
	chanacs_t *ca;
	node_t *n;
	sra_t *sra;
	mychan_t *mc;
	kline_t *k;
	int retval;
	uint32_t i = 0, muin = 0, mcin = 0, kin = 0;
	uint32_t dbo_port;
	char dbcredentials[BUFSIZE];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;

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
		slog(LG_DEBUG, "There was an error connecting to the database:");
		slog(LG_DEBUG, "    %s", mysql_error(mysql));

		exit(1);
	}

	if((retval = mysql_select_db(mysql, database_options.database)))
	{
		slog(LG_DEBUG, "There was an error selecting the database:");
		slog(LG_DEBUG, "    %s", mysql_error(mysql));

		exit(1);
	}

	res = safe_query("SELECT * FROM ACCOUNTS");
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

		res2 = safe_query("SELECT * FROM ACCOUNT_METADATA WHERE PARENT=%d", uid);
		umd = mysql_num_rows(res2);

		while ((row2 = mysql_fetch_row(res2)))
			metadata_add(mu, METADATA_USER, row2[2], row2[3]);

		mysql_free_result(res2);

		res2 = safe_query("SELECT * FROM ACCOUNT_MEMOS WHERE PARENT=%d", uid);
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

                        node_add(mz, node_create(), &mu->memos);
		}
	}

	mysql_free_result(res);
	res = NULL;

	res = safe_query("SELECT * FROM CHANNELS");
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
		}

		mc->registered = atoi(row[3]);
		mc->used = atoi(row[4]);
		mc->flags = atoi(row[5]);

		mc->mlock_on = atoi(row[6]);
		mc->mlock_off = atoi(row[7]);
		mc->mlock_limit = atoi(row[8]);
		mc->mlock_key = sstrdup(row[9]);

		if (row[10] && (*row[10] != '\0'))
			metadata_add(mc, METADATA_CHANNEL, "url", row[10]);
		if (row[11] && (*row[11] != '\0'))
			metadata_add(mc, METADATA_CHANNEL, "private:entrymsg", row[11]);


		/* SELECT * FROM CHANNEL_ACCESS WHERE PARENT=21 */
		res2 = safe_query("SELECT * WHERE PARENT='%s'", row[0]);

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

			res3 = safe_query("SELECT * FROM CHANNEL_ACCESS_METADATA WHERE PARENT='%s'", row2[0]);

			ca_mdin = mysql_num_rows(res3);
			while ((row3 = mysql_fetch_row(res3)))
				metadata_add(ca, METADATA_CHANACS, row3[2], row3[3]);

			mysql_free_result(res3);
			res3 = NULL;
		}
		mysql_free_result(res2);
		res2 = NULL;

		/* SELECT * FROM CHANNEL_METADATA WHERE PARENT=21 */
		res2 = safe_query("SELECT * FROM CHANNEL_METADATA WHERE PARENT='%s'", row[0]);
		mdin = mysql_num_rows(res2);

		while ((row2 = mysql_fetch_row(res2)))
			metadata_add(mc, METADATA_CHANNEL, row[2], row[3]);

		mysql_free_result(res2);
		res2 = NULL;
	}

	mysql_free_result(res);
	res = NULL;

	res = safe_query("SELECT * FROM KLINES");
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

	LIST_FOREACH(n, sralist.head)
	{
	    sra = (sra_t *)n->data;

		if (!sra->myuser)
		{
			sra->myuser = myuser_find(sra->name);

			if (sra->myuser)
			{
				slog(LG_DEBUG, "db_load(): updating %s to SRA", sra->name);
				sra->myuser->sra = sra;
			}
		}
	}

	mysql_close(mysql);
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &mysql_db_load;
	db_save = &mysql_db_save;

	backend_loaded = TRUE;
}
