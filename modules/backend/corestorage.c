/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 *
 * This contains the core storage components, which comprises basically
 * everything which used to live in atheme.db a long time ago.  It is not
 * tied to any specific storage engine, but the Atheme storage APIs assume
 * that all database systems are similar to OpenSEX (which is basically like
 * flatfile, but on acid, e.g. assumptions that the database is layered out like
 * a set of non-finite table cells).
 *
 * The stuff here should be moved to the individual components which depend
 * on them.  e.g. struct myuser should register their own writer, struct mychan
 * same story etc.
 *
 *    --nenolod
 */

#include <atheme.h>

// MDEPs to write to the database on commit, for reloading on startup
#define MODFLAG_PRIV_MDEP       (MODFLAG_DBCRYPTO | MODFLAG_DBHANDLER)

static unsigned int dbv;
static unsigned int their_ca_all;

static time_t db_time;

static bool mdep_load_mdeps = true;

#ifdef HAVE_FORK
static pid_t child_pid;
#endif

// write atheme.db (core fields)
static void
corestorage_db_save(struct database_handle *db)
{
	struct metadata *md;
	struct myuser *mu;
	struct myentity *ment;
	struct myuser_name *mun;
	struct mychan *mc;
	struct chanacs *ca;
	struct kline *k;
	struct xline *x;
	struct qline *q;
	struct svsignore *svsignore;
	struct soper *soper;
	mowgli_node_t *n, *tn;
	mowgli_patricia_iteration_state_t state;
	struct myentity_iteration_state mestate;

	errno = 0;

	// write the database version
	db_start_row(db, "DBV");
	db_write_uint(db, 12);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH(n, modules.head)
	{
		const struct module *const m = n->data;

		if (! (m->mflags & MODFLAG_PRIV_MDEP))
			continue;

		db_start_row(db, "MDEP");
		db_write_word(db, m->name);
		db_commit_row(db);
	}

	db_start_row(db, "LUID");
	db_write_word(db, myentity_get_last_uid());
	db_commit_row(db);

	db_start_row(db, "CF");
	db_write_word(db, bitmask_to_flags(ca_all));
	db_commit_row(db);

	db_start_row(db, "TS");
	db_write_time(db, CURRTIME);
	db_commit_row(db);

	slog(LG_DEBUG, "db_save(): saving myusers");

	MYENTITY_FOREACH_T(ment, &mestate, ENT_USER)
	{
		mu = user(ment);
		/* MU <name> <pass> <email> <registered> <lastlogin> <failnum*> <lastfail*>
		 * <lastfailon*> <flags> <language>
		 *
		 *  * failnum, lastfail, and lastfailon are deprecated (moved to metadata)
		 */
		char *flags = gflags_tostr(mu_flags, MOWGLI_LIST_LENGTH(&mu->logins) ? mu->flags & ~MU_NOBURSTLOGIN : mu->flags);
		db_start_row(db, "MU");
		db_write_word(db, entity(mu)->id);
		db_write_word(db, entity(mu)->name);
		db_write_word(db, mu->pass);
		db_write_word(db, mu->email);
		db_write_time(db, mu->registered);

		if (MOWGLI_LIST_LENGTH(&mu->logins))
			db_write_time(db, 0);
		else
			db_write_time(db, mu->lastlogin);

		db_write_word(db, flags);
		db_write_word(db, language_get_name(mu->language));
		db_commit_row(db);

		if (atheme_object(mu)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state, atheme_object(mu)->metadata)
			{
				db_start_row(db, "MDU");
				db_write_word(db, entity(mu)->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}

		MOWGLI_ITER_FOREACH(tn, mu->memos.head)
		{
			struct mymemo *mz = (struct mymemo *)tn->data;

			db_start_row(db, "ME");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, mz->sender);
			db_write_time(db, mz->sent);
			db_write_uint(db, mz->status);
			db_write_str(db, mz->text);
			db_commit_row(db);
		}

		MOWGLI_ITER_FOREACH(tn, mu->memo_ignores.head)
		{
			db_start_row(db, "MI");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, (char *)tn->data);
			db_commit_row(db);
		}

		MOWGLI_ITER_FOREACH(tn, mu->access_list.head)
		{
			db_start_row(db, "AC");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, (char *)tn->data);
			db_commit_row(db);
		}

		MOWGLI_ITER_FOREACH(tn, mu->nicks.head)
		{
			struct mynick *mn = tn->data;

			db_start_row(db, "MN");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, mn->nick);
			db_write_time(db, mn->registered);

			struct user *u = user_find_named(mn->nick);
			if (u != NULL && u->myuser == mn->owner)
				db_write_time(db, 0);
			else
				db_write_time(db, mn->lastseen);

			db_commit_row(db);
		}

		MOWGLI_ITER_FOREACH(tn, mu->cert_fingerprints.head)
		{
			struct mycertfp *mcfp = tn->data;

			db_start_row(db, "MCFP");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, mcfp->certfp);
			db_commit_row(db);
		}
	}

	// XXX: groupserv hack.  remove when we have proper dependency resolution. --nenolod
	hook_call_db_write_pre_ca(db);

	slog(LG_DEBUG, "db_save(): saving mychans");

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		mowgli_patricia_iteration_state_t state2;

		char *flags = gflags_tostr(mc_flags, mc->flags);

		// find a founder
		mu = NULL;
		MOWGLI_ITER_FOREACH(tn, mc->chanacs.head)
		{
			ca = (struct chanacs *)tn->data;
			if (ca->entity != NULL && ca->level & CA_FOUNDER)
			{
				mu = user(ca->entity);
				break;
			}
		}

		// MC <name> <registered> <used> <flags> <mlock_on> <mlock_off> <mlock_limit> [mlock_key]
		db_start_row(db, "MC");
		db_write_word(db, mc->name);
		db_write_time(db, mc->registered);
		db_write_time(db, mc->used);
		db_write_word(db, flags);
		db_write_uint(db, mc->mlock_on);
		db_write_uint(db, mc->mlock_off);
		db_write_uint(db, mc->mlock_limit);
		db_write_word(db, mc->mlock_key ? mc->mlock_key : "");
		db_commit_row(db);

		MOWGLI_ITER_FOREACH(tn, mc->chanacs.head)
		{
			struct myentity *setter = NULL;
			ca = (struct chanacs *)tn->data;

			db_start_row(db, "CA");
			db_write_word(db, ca->mychan->name);
			db_write_word(db, ca->entity ? ca->entity->name : ca->host);
			db_write_word(db, bitmask_to_flags(ca->level));
			db_write_time(db, ca->tmodified);

			if (*ca->setter_uid != '\0' && (setter = myentity_find_uid(ca->setter_uid)))
				db_write_word(db, setter->name);
			else
				db_write_word(db, "*");

			db_commit_row(db);

			if (atheme_object(ca)->metadata)
			{
				MOWGLI_PATRICIA_FOREACH(md, &state2, atheme_object(ca)->metadata)
				{
					db_start_row(db, "MDA");
					db_write_word(db, ca->mychan->name);
					db_write_word(db, (ca->entity) ? ca->entity->name : ca->host);
					db_write_word(db, md->name);
					db_write_str(db, md->value);
					db_commit_row(db);
				}
			}
		}

		if (atheme_object(mc)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state2, atheme_object(mc)->metadata)
			{
				db_start_row(db, "MDC");
				db_write_word(db, mc->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}
	}

	// Old names
	MOWGLI_PATRICIA_FOREACH(mun, &state, oldnameslist)
	{
		mowgli_patricia_iteration_state_t state2;

		db_start_row(db, "NAM");
		db_write_word(db, mun->name);
		db_commit_row(db);

		if (atheme_object(mun)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state2, atheme_object(mun)->metadata)
			{
				db_start_row(db, "MDN");
				db_write_word(db, mun->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}
	}

	// Services ignores
	slog(LG_DEBUG, "db_save(): saving svsignores");

	MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (struct svsignore *)n->data;

		// SI <mask> <settime> <setby> <reason>
		db_start_row(db, "SI");
		db_write_word(db, svsignore->mask);
		db_write_time(db, svsignore->settime);
		db_write_word(db, svsignore->setby);
		db_write_str(db, svsignore->reason);
		db_commit_row(db);
	}

	// Services operators
	slog(LG_DEBUG, "db_save(): saving sopers");

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		const char *flags;
		soper = n->data;
		flags = gflags_tostr(soper_flags, soper->flags);

		if (soper->flags & SOPER_CONF || soper->myuser == NULL)
			continue;

		// SO <account> <operclass> <flags> [password]
		db_start_row(db, "SO");
		db_write_word(db, entity(soper->myuser)->name);
		db_write_word(db, soper->classname);
		db_write_word(db, flags);

		if (soper->password != NULL)
			db_write_word(db, soper->password);

		db_commit_row(db);
	}

	slog(LG_DEBUG, "db_save(): saving klines");

	db_start_row(db, "KID");
	db_write_uint(db, me.kline_id);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH(n, klnlist.head)
	{
		k = (struct kline *)n->data;

		// KL <user> <host> <duration> <settime> <setby> <reason>
		db_start_row(db, "KL");
		db_write_uint(db, k->number);
		db_write_word(db, k->user);
		db_write_word(db, k->host);
		db_write_uint(db, k->duration);
		db_write_time(db, k->settime);
		db_write_word(db, k->setby);
		db_write_str(db, k->reason);
		db_commit_row(db);
	}

	slog(LG_DEBUG, "db_save(): saving xlines");

	db_start_row(db, "XID");
	db_write_uint(db, me.xline_id);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH(n, xlnlist.head)
	{
		x = (struct xline *)n->data;

		// XL <gecos> <duration> <settime> <setby> <reason>
		db_start_row(db, "XL");
		db_write_uint(db, x->number);
		db_write_word(db, x->realname);
		db_write_uint(db, x->duration);
		db_write_time(db, x->settime);
		db_write_word(db, x->setby);
		db_write_str(db, x->reason);
		db_commit_row(db);
	}

	db_start_row(db, "QID");
	db_write_uint(db, me.qline_id);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		// QL <mask> <duration> <settime> <setby> <reason>
		db_start_row(db, "QL");
		db_write_uint(db, q->number);
		db_write_word(db, q->mask);
		db_write_uint(db, q->duration);
		db_write_time(db, q->settime);
		db_write_word(db, q->setby);
		db_write_str(db, q->reason);
		db_commit_row(db);
	}
}

static void ATHEME_FATTR_NORETURN
corestorage_h_unknown(struct database_handle *db, const char *type)
{
	slog(LG_ERROR, "db %s:%u: unknown directive '%s'", db->file, db->line, type);
	slog(LG_ERROR, "corestorage: exiting to avoid data loss");
	exit(EXIT_FAILURE);
}

static void
corestorage_h_dbv(struct database_handle *db, const char *type)
{
	dbv = db_sread_uint(db);
	slog(LG_INFO, "corestorage: data schema version is %u.", dbv);
}

static void
corestorage_h_mdep(struct database_handle *const restrict db, const char ATHEME_VATTR_UNUSED *const restrict type)
{
	if (! mdep_load_mdeps)
		return;

	const char *const modname = db_sread_word(db);

	if (! modname || ! *modname)
		return;

	if (strncmp(modname, "transport/", 10) == 0 || strncmp(modname, "protocol/", 9) == 0)
	{
		// Old (<= 7.2) database
		mdep_load_mdeps = false;
		return;
	}

	if (module_find_published(modname))
		return;

	if (config_options.load_database_mdeps)
	{
		(void) slog(LG_INFO, "corestorage: auto-loading module '%s' (database depends on it)", modname);

		if (module_request(modname))
			return;

		(void) slog(LG_ERROR, "db %s:%u: cannot load module '%s'", db->file, db->line, modname);
	}
	else
		(void) slog(LG_ERROR, "db %s:%u: need module '%s' to process the database successfully, but have "
		                      "been configured to not load any (general::load_database_mdeps)", db->file,
		                      db->line, modname);

	(void) slog(LG_ERROR, "corestorage: exiting to avoid data loss");

	exit(EXIT_FAILURE);
}

static void
corestorage_h_luid(struct database_handle *db, const char *type)
{
	myentity_set_last_uid(db_sread_word(db));
}

static void
corestorage_h_cf(struct database_handle *db, const char *type)
{
	const char *flags = db_sread_word(db);

	their_ca_all = flags_to_bitmask(flags, 0);
	if (their_ca_all & ~ca_all)
	{
		slog(LG_ERROR, "db-h-cf: losing flags %s from file", bitmask_to_flags(their_ca_all & ~ca_all));
	}
	if (~their_ca_all & ca_all)
	{
		slog(LG_ERROR, "db-h-cf: making up flags %s not present in file", bitmask_to_flags(~their_ca_all & ca_all));
	}
}

static void
corestorage_h_ts(struct database_handle *db, const char *type)
{
	db_time = db_sread_time(db);
}

static void
corestorage_h_mu(struct database_handle *db, const char *type)
{
	const char *uid = NULL;
	const char *name;
	const char *pass, *email, *language;
	time_t reg, login;
	const char *sflags;
	unsigned int flags = 0;
	struct myuser *mu;

	if (dbv >= 10)
		uid = db_sread_word(db);

	name = db_sread_word(db);

	if (myuser_find(name))
	{
		slog(LG_INFO, "db-h-mu: line %u: skipping duplicate account %s", db->line, name);
		return;
	}

	if (strict_mode && uid && myuser_find_uid(uid))
	{
		slog(LG_INFO, "db-h-mu: line %u: skipping account %s with duplicate UID %s", db->line, name, uid);
		return;
	}

	pass = db_sread_word(db);
	email = db_sread_word(db);
	reg = db_sread_time(db);
	login = db_sread_time(db);
	if (dbv >= 8) {
		sflags = db_sread_word(db);
		if (!gflags_fromstr(mu_flags, sflags, &flags))
			slog(LG_INFO, "db-h-mu: line %u: confused by flags: %s", db->line, sflags);
	} else {
		flags = db_sread_uint(db);
	}
	language = db_read_word(db);


	mu = myuser_add_id(uid, name, pass, email, flags);
	mu->registered = reg;

	if (login != 0)
		mu->lastlogin = login;
	else if (db_time != 0)
		mu->lastlogin = db_time;
	else
		/* we'll only get here if someone manually corrupted a DB */
		mu->lastlogin = CURRTIME;

	if (language)
		mu->language = language_add(language);
}

static void
corestorage_h_me(struct database_handle *db, const char *type)
{
	const char *dest, *src, *text;
	time_t sent;
	unsigned int status;
	struct myuser *mu;
	struct mymemo *mz;

	dest = db_sread_word(db);
	src = db_sread_word(db);
	sent = db_sread_time(db);
	status = db_sread_uint(db);
	text = db_sread_str(db);

	if (!(mu = myuser_find(dest)))
	{
		slog(LG_DEBUG, "db-h-me: line %u: memo for unknown account %s", db->line, dest);
		return;
	}

	mz = smalloc(sizeof *mz);
	mowgli_strlcpy(mz->sender, src, sizeof mz->sender);
	mowgli_strlcpy(mz->text, text, sizeof mz->text);
	mz->sent = sent;
	mz->status = status;

	if (!(mz->status & MEMO_READ))
		mu->memoct_new++;

	mowgli_node_add(mz, mowgli_node_create(), &mu->memos);
}

static void
corestorage_h_mi(struct database_handle *db, const char *type)
{
	struct myuser *mu;
	const char *user, *target;

	user = db_sread_word(db);
	target = db_sread_word(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-mi: line %u: ignore for unknown account %s", db->line, user);
		return;
	}

	mowgli_node_add(sstrdup(target), mowgli_node_create(), &mu->memo_ignores);
}

static void
corestorage_h_ac(struct database_handle *db, const char *type)
{
	struct myuser *mu;
	const char *user, *mask;

	user = db_sread_word(db);
	mask = db_sread_word(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-ac: line %u: access entry for unknown account %s", db->line, user);
		return;
	}

	myuser_access_add(mu, mask);
}

static void
corestorage_h_mn(struct database_handle *db, const char *type)
{
	struct myuser *mu;
	struct mynick *mn;
	const char *user, *nick;
	time_t reg, seen;

	user = db_sread_word(db);
	nick = db_sread_word(db);
	reg = db_sread_time(db);
	seen = db_sread_time(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-mn: line %u: registered nick %s for unknown account %s", db->line, nick, user);
		return;
	}

	if (mynick_find(nick))
	{
		slog(LG_INFO, "db-h-mn: line %u: skipping duplicate nick %s for account %s", db->line, nick, user);
		return;
	}

	mn = mynick_add(mu, nick);
	mn->registered = reg;

	if (seen != 0)
		mn->lastseen = seen;
	else if (db_time != 0)
		mn->lastseen = db_time;
	else
		/* we'll only get here if someone manually corrupted a DB */
		mn->lastseen = CURRTIME;
}

static void
corestorage_h_mcfp(struct database_handle *db, const char *type)
{
	const char *user, *certfp;
	struct myuser *mu;

	user = db_sread_word(db);
	certfp = db_sread_word(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-mcfp: certfp %s for unknown account %s", certfp, user);
		return;
	}

	mycertfp_add(mu, certfp, true);
}

static void
corestorage_h_su(struct database_handle *db, const char *type)
{
	slog(LG_INFO, "db-h-su: line %u: metadata change subscriptions have been dropped, ignoring", db->line);
}

static void
corestorage_h_nam(struct database_handle *db, const char *type)
{
	const char *user = db_sread_word(db);
	myuser_name_add(user);
}

static void
corestorage_h_so(struct database_handle *db, const char *type)
{
	const char *user, *class, *pass;
	unsigned int flags = 0;
	struct myuser *mu;
	const char *sflags;

	user = db_sread_word(db);
	class = db_sread_word(db);
	if (dbv >= 8)
	{
		sflags = db_sread_word(db);
		if (!gflags_fromstr(soper_flags, sflags, &flags))
			slog(LG_INFO, "db-h-so: line %u: confused by flags %s",
			     db->line, sflags);
	} else {
		flags = db_sread_uint(db);
	}
	pass = db_read_word(db);
	if (pass != NULL && !*pass)
		pass = NULL;

	if (!(mu = myuser_find(user)))
	{
		slog(LG_ERROR, "db-h-so: soper for nonexistent account %s", user);
		return;
	}
	if (!operclass_find(class))
	{
		slog(LG_ERROR, "db-h-so: soper for nonexistent class %s", class);
		return;
	}

	soper_add(entity(mu)->name, class, flags & ~SOPER_CONF, pass);
}

static void
corestorage_h_mc(struct database_handle *db, const char *type)
{
	char buf[4096];
	const char *name = db_sread_word(db);
	const char *key;
	const char *sflags;
	unsigned int flags = 0;

	mowgli_strlcpy(buf, name, sizeof buf);
	struct mychan *mc = mychan_add(buf);

	mc->registered = db_sread_time(db);
	mc->used = db_sread_time(db);
	if (dbv >= 8) {
		sflags = db_sread_word(db);
		if (!gflags_fromstr(mc_flags, sflags, &flags))
			slog(LG_INFO, "db-h-mc: line %u: confused by flags %s",
			     db->line, sflags);
	} else {
		flags = db_sread_uint(db);
	}

	mc->flags = flags;
	mc->mlock_on = db_sread_uint(db);
	mc->mlock_off = db_sread_uint(db);
	mc->mlock_limit = db_sread_uint(db);

	if ((key = db_read_word(db)))
	{
		mowgli_strlcpy(buf, key, sizeof buf);
		strip(buf);
		if (buf[0] && buf[0] != ':' && !strchr(buf, ','))
			mc->mlock_key = sstrdup(buf);
	}
}

static char * ATHEME_FATTR_MALLOC
convert_templates(const char *value)
{
	char *newvalue, *p;
	size_t len;

	len = strlen(value);
	newvalue = smalloc(2 * len + 1);
	p = newvalue;
	for (;;)
	{
		while (*value != '\0' && *value != '=')
			*p++ = *value++;
		if (*value == '\0')
			break;
		while (*value != '\0' && *value != ' ')
		{
			if (*value == 'r')
				*p++ = 'e';
			*p++ = *value++;
		}
		if (*value == '\0')
			break;
	}
	*p = '\0';
	return newvalue;
}

static void
corestorage_h_md(struct database_handle *db, const char *type)
{
	const char *name = db_sread_word(db);
	const char *prop = db_sread_word(db);
	const char *value = db_sread_str(db);
	char *newvalue = NULL;
	void *obj = NULL;

	if (!strcmp(type, "MDU"))
	{
		obj = myuser_find(name);
	}
	else if (!strcmp(type, "MDC"))
	{
		obj = mychan_find(name);
		if (!(their_ca_all & CA_EXEMPT) &&
				!strcmp(prop, "private:templates"))
		{
			newvalue = convert_templates(value);
			value = newvalue;
		}
	}
	else if (!strcmp(type, "MDA"))
	{
		char *mask = strrchr(name, ':');
		if (mask != NULL)
		{
			*mask++ = '\0';
			obj = chanacs_find_by_mask(mychan_find(name), mask, CA_NONE);
		}
	}
	else if (!strcmp(type, "MDN"))
	{
		obj = myuser_name_find(name);
	}
	else
	{
		slog(LG_INFO, "db-h-md: unknown metadata type '%s'; name %s, prop %s", type, name, prop);
		return;
	}

	if (obj == NULL)
	{
		slog(LG_INFO, "db-h-md: attempting to add %s property to non-existant object %s",
		     prop, name);
		sfree(newvalue);
		return;
	}

	metadata_add(obj, prop, value);
	sfree(newvalue);
}

static void
corestorage_h_mda(struct database_handle *db, const char *type)
{
	const char *name, *prop, *value, *mask;
	void *obj = NULL;

	if (dbv < 12)
		return corestorage_h_md(db, type);

	name = db_sread_word(db);
	mask = db_sread_word(db);
	prop = db_sread_word(db);
	value = db_sread_str(db);

	obj = chanacs_find_by_mask(mychan_find(name), mask, CA_NONE);

	if (obj == NULL)
	{
		slog(LG_INFO, "db-h-mda: attempting to add %s property to non-existant object %s (acl %s)",
		     prop, name, mask);
		return;
	}

	metadata_add(obj, prop, value);
}

static void
corestorage_h_ca(struct database_handle *db, const char *type)
{
	const char *chan, *target;
	time_t tmod;
	unsigned int flags;
	struct mychan *mc;
	struct myentity *mt;
	struct myentity *setter;

	chan = db_sread_word(db);
	target = db_sread_word(db);
	flags = flags_to_bitmask(db_sread_word(db), 0);

	// UNBAN self and akick exempt have been split to +e per GitHub #75
	if (!(their_ca_all & CA_EXEMPT) && (flags & CA_REMOVE))
		flags |= CA_EXEMPT;

	tmod = db_sread_time(db);

	mc = mychan_find(chan);
	mt = myentity_find(target);

	setter = NULL;
	if (dbv >= 9)
		setter = myentity_find(db_sread_word(db));

	if (mc == NULL)
	{
		slog(LG_INFO, "db-h-ca: line %u: chanacs for nonexistent channel %s - exiting to avoid data loss", db->line, chan);
		slog(LG_INFO, "db-h-ca: line %u: if this depends on a specific module or feature; please make sure", db->line);
		slog(LG_INFO, "db-h-ca: line %u: that feature is enabled.", db->line);
		exit(EXIT_FAILURE);
	}

	if (mt == NULL && !validhostmask(target))
	{
		slog(LG_INFO, "db-h-ca: line %u: chanacs for nonexistent target %s - exiting to avoid data loss", db->line, target);
		slog(LG_INFO, "db-h-ca: line %u: if this depends on a specific module or feature; please make sure", db->line);
		slog(LG_INFO, "db-h-ca: line %u: that feature is enabled.", db->line);
		exit(EXIT_FAILURE);
	}

	if (mt == NULL && validhostmask(target))
	{
		chanacs_add_host(mc, target, flags, tmod, setter);
	}
	else
	{
		chanacs_add(mc, mt, flags, tmod, setter);
	}
}

static void
corestorage_h_si(struct database_handle *db, const char *type)
{
	char buf[4096];
	const char *mask, *setby, *reason;
	time_t settime;
	struct svsignore *svsignore;

	mask = db_sread_word(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);
	mowgli_strlcpy(buf, reason, sizeof buf);

	strip(buf);
	svsignore = svsignore_add(mask, reason);
	svsignore->settime = settime;
	svsignore->setby = sstrdup(setby);
}

static void
corestorage_h_kid(struct database_handle *db, const char *type)
{
	me.kline_id = db_sread_uint(db);
}

static void
corestorage_h_kl(struct database_handle *db, const char *type)
{
	char buf[4096];
	const char *user, *host, *reason, *setby;
	unsigned int id = 0;
	time_t settime;
	long duration;
	struct kline *k;

	if (dbv > 10)
		id = db_sread_uint(db);

	user = db_sread_word(db);
	host = db_sread_word(db);
	duration = db_sread_uint(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);

	mowgli_strlcpy(buf, reason, sizeof buf);
	strip(buf);

	k = kline_add_with_id(user, host, buf, duration, setby, id ? id : ++me.kline_id);
	k->settime = settime;
	k->expires = k->settime + k->duration;
}

static void
corestorage_h_xid(struct database_handle *db, const char *type)
{
	me.xline_id = db_sread_uint(db);
}

static void
corestorage_h_xl(struct database_handle *db, const char *type)
{
	char buf[4096];
	const char *realname, *reason, *setby;
	unsigned int id = 0;
	time_t settime;
	long duration;
	struct xline *x;

	if (dbv > 10)
		id = db_sread_uint(db);

	realname = db_sread_word(db);
	duration = db_sread_uint(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);

	mowgli_strlcpy(buf, reason, sizeof buf);
	strip(buf);

	x = xline_add(realname, buf, duration, setby);
	x->settime = settime;
	x->expires = x->settime + x->duration;

	if (id)
		x->number = id;
}

static void
corestorage_h_qid(struct database_handle *db, const char *type)
{
	me.qline_id = db_sread_uint(db);
}

static void
corestorage_h_ql(struct database_handle *db, const char *type)
{
	char buf[4096];
	const char *mask, *reason, *setby;
	unsigned int id = 0;
	time_t settime;
	long duration;
	struct qline *q;

	if (dbv > 10)
		id = db_sread_uint(db);

	mask = db_sread_word(db);
	duration = db_sread_uint(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);

	mowgli_strlcpy(buf, reason, sizeof buf);
	strip(buf);

	q = qline_add(mask, buf, duration, setby);
	q->settime = settime;
	q->expires = q->settime + q->duration;

	if (id)
		q->number = id;
}

static void
corestorage_ignore_row(struct database_handle *db, const char *type)
{
	return;
}

static void
corestorage_db_load(const char *filename)
{
	struct database_handle *db;

	db = db_open(filename, DB_READ);
	if (db == NULL)
		return;

	db_time = 0;

	db_parse(db);
	db_close(db);
}

static void
corestorage_db_write_blocking(void *filename)
{
	struct database_handle *db;

	db = db_open(filename, DB_WRITE);

	if (! db)
	{
		slog(LG_ERROR, "db_write_blocking(): db_open() failed, aborting save");
		return;
	}

	corestorage_db_save(db);
	hook_call_db_write(db);

	db_close(db);
}

#ifdef HAVE_FORK
static void
corestorage_db_saved_cb(pid_t pid, int status, void *data)
{
	if (child_pid != pid)
		return; // probably killed our child for a forced write
	else
	{
		child_pid = 0;
		slog(LG_DEBUG, "db_save(): finished asynchronous DB write");
	}
}
#endif

static void
corestorage_db_write(void *filename, enum db_save_strategy strategy)
{
#ifndef HAVE_FORK
	corestorage_db_write_blocking(filename);
#else

	if (child_pid && strategy == DB_SAVE_BG_REGULAR)
	{
		slog(LG_DEBUG, "db_save(): previous save unfinished, skipping save");
		return;
	}

	if (child_pid)
	{
		slog(LG_DEBUG, "db_save(): interrupting unfinished previous save for forced save");
		if (kill(child_pid, SIGKILL) == -1 && errno != ESRCH)
		{
			slog(LG_ERROR, "db_save(): kill() on previous save failed; trying to carry on somehow...");
			waitpid(child_pid, NULL, 0);
		}
	}

	if (strategy == DB_SAVE_BLOCKING)
	{
		corestorage_db_write_blocking(filename);
		return;
	}

	pid_t pid = fork();
	switch (pid)
	{
		case -1:
			slog(LG_ERROR, "db_save(): fork() failed; writing database synchronously");
			corestorage_db_write_blocking(filename);
			return;

		case 0:
			corestorage_db_write_blocking(filename);
			exit(EXIT_SUCCESS);

		default:
			child_pid = pid;
			childproc_add(pid, "db_save", corestorage_db_saved_cb, NULL);
			return;
	}
#endif
}

static void
mod_init(struct module *const restrict m)
{
	db_load = &corestorage_db_load;
	db_save = &corestorage_db_write;

	db_register_type_handler("DBV", corestorage_h_dbv);
	db_register_type_handler("MDEP", corestorage_h_mdep);
	db_register_type_handler("LUID", corestorage_h_luid);
	db_register_type_handler("CF", corestorage_h_cf);
	db_register_type_handler("TS", corestorage_h_ts);
	db_register_type_handler("MU", corestorage_h_mu);
	db_register_type_handler("ME", corestorage_h_me);
	db_register_type_handler("MI", corestorage_h_mi);
	db_register_type_handler("AC", corestorage_h_ac);
	db_register_type_handler("MN", corestorage_h_mn);
	db_register_type_handler("MCFP", corestorage_h_mcfp);
	db_register_type_handler("SU", corestorage_h_su);
	db_register_type_handler("NAM", corestorage_h_nam);
	db_register_type_handler("SO", corestorage_h_so);
	db_register_type_handler("MC", corestorage_h_mc);
	db_register_type_handler("MDU", corestorage_h_md);
	db_register_type_handler("MDC", corestorage_h_md);
	db_register_type_handler("MDA", corestorage_h_mda);
	db_register_type_handler("MDN", corestorage_h_md);
	db_register_type_handler("CA", corestorage_h_ca);
	db_register_type_handler("SI", corestorage_h_si);

	db_register_type_handler("KID", corestorage_h_kid);
	db_register_type_handler("KL", corestorage_h_kl);
	db_register_type_handler("XID", corestorage_h_xid);
	db_register_type_handler("XL", corestorage_h_xl);
	db_register_type_handler("QID", corestorage_h_qid);
	db_register_type_handler("QL", corestorage_h_ql);

	db_register_type_handler("DE", corestorage_ignore_row);

	db_register_type_handler("???", corestorage_h_unknown);

	backend_loaded = true;

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("backend/corestorage", MODULE_UNLOAD_CAPABILITY_NEVER)
