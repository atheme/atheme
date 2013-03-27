/*
 * Copyright (c) 2005-2012 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This contains the core storage components, which comprises basically
 * everything which used to live in atheme.db a long time ago.  It is not
 * tied to any specific storage engine, but the Atheme storage APIs assume
 * that all database systems are similar to OpenSEX (which is basically like
 * flatfile, but on acid, e.g. assumptions that the database is layered out like
 * a set of non-finite table cells).
 *
 * The stuff here should be moved to the individual components which depend
 * on them.  e.g. myuser_t should register their own writer, mychan_t same
 * story etc.
 *
 *    --nenolod
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"backend/corestorage", true, _modinit, NULL,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

unsigned int dbv;
unsigned int their_ca_all;

extern mowgli_list_t modules;

/* write atheme.db (core fields) */
static void
corestorage_db_save(database_handle_t *db)
{
	metadata_t *md;
	myuser_t *mu;
	myentity_t *ment;
	myuser_name_t *mun;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	xline_t *x;
	qline_t *q;
	svsignore_t *svsignore;
	soper_t *soper;
	mowgli_node_t *n, *tn;
	mowgli_patricia_iteration_state_t state;
	myentity_iteration_state_t mestate;

	errno = 0;

	/* write the database version */
	db_start_row(db, "DBV");
	db_write_int(db, 12);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

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
		db_write_time(db, mu->lastlogin);
		db_write_word(db, flags);
		db_write_word(db, language_get_name(mu->language));
		db_commit_row(db);

		if (object(mu)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state, object(mu)->metadata)
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
			mymemo_t *mz = (mymemo_t *)tn->data;

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
			mynick_t *mn = tn->data;

			db_start_row(db, "MN");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, mn->nick);
			db_write_time(db, mn->registered);
			db_write_time(db, mn->lastseen);
			db_commit_row(db);
		}

		MOWGLI_ITER_FOREACH(tn, mu->cert_fingerprints.head)
		{
			mycertfp_t *mcfp = tn->data;

			db_start_row(db, "MCFP");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, mcfp->certfp);
			db_commit_row(db);
		}
	}

	/* XXX: groupserv hack.  remove when we have proper dependency resolution. --nenolod */
	hook_call_db_write_pre_ca(db);

	slog(LG_DEBUG, "db_save(): saving mychans");

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		mowgli_patricia_iteration_state_t state2;

		char *flags = gflags_tostr(mc_flags, mc->flags);
		/* find a founder */
		mu = NULL;
		MOWGLI_ITER_FOREACH(tn, mc->chanacs.head)
		{
			ca = (chanacs_t *)tn->data;
			if (ca->entity != NULL && ca->level & CA_FOUNDER)
			{
				mu = user(ca->entity);
				break;
			}
		}
		/* MC <name> <registered> <used> <flags> <mlock_on> <mlock_off> <mlock_limit> [mlock_key] */
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
			ca = (chanacs_t *)tn->data;

			db_start_row(db, "CA");
			db_write_word(db, ca->mychan->name);
			db_write_word(db, ca->entity ? ca->entity->name : ca->host);
			db_write_word(db, bitmask_to_flags(ca->level));
			db_write_time(db, ca->tmodified);
			db_write_word(db, ca->setter ? ca->setter : "*");
			db_commit_row(db);

			if (object(ca)->metadata)
			{
				MOWGLI_PATRICIA_FOREACH(md, &state2, object(ca)->metadata)
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

		if (object(mc)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state2, object(mc)->metadata)
			{
				db_start_row(db, "MDC");
				db_write_word(db, mc->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}
	}

	/* Old names */
	MOWGLI_PATRICIA_FOREACH(mun, &state, oldnameslist)
	{
		mowgli_patricia_iteration_state_t state2;

		db_start_row(db, "NAM");
		db_write_word(db, mun->name);
		db_commit_row(db);

		if (object(mun)->metadata)
		{
			MOWGLI_PATRICIA_FOREACH(md, &state2, object(mun)->metadata)
			{
				db_start_row(db, "MDN");
				db_write_word(db, mun->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}
	}

	/* Services ignores */
	slog(LG_DEBUG, "db_save(): saving svsignores");

	MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		/* SI <mask> <settime> <setby> <reason> */
		db_start_row(db, "SI");
		db_write_word(db, svsignore->mask);
		db_write_time(db, svsignore->settime);
		db_write_word(db, svsignore->setby);
		db_write_str(db, svsignore->reason);
		db_commit_row(db);
	}

	/* Services operators */
	slog(LG_DEBUG, "db_save(): saving sopers");

	MOWGLI_ITER_FOREACH(n, soperlist.head)
	{
		const char *flags;
		soper = n->data;
		flags = gflags_tostr(soper_flags, soper->flags);

		if (soper->flags & SOPER_CONF || soper->myuser == NULL)
			continue;

		/* SO <account> <operclass> <flags> [password] */
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
		k = (kline_t *)n->data;

		/* KL <user> <host> <duration> <settime> <setby> <reason> */
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
		x = (xline_t *)n->data;

		/* XL <gecos> <duration> <settime> <setby> <reason> */
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
		q = (qline_t *)n->data;

		/* QL <mask> <duration> <settime> <setby> <reason> */
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

static void corestorage_h_unknown(database_handle_t *db, const char *type)
{
	slog(LG_ERROR, "db %s:%d: unknown directive '%s'", db->file, db->line, type);
	slog(LG_ERROR, "corestorage: exiting to avoid data loss");
	exit(EXIT_FAILURE);
}

static void corestorage_h_dbv(database_handle_t *db, const char *type)
{
	dbv = db_sread_int(db);
	slog(LG_INFO, "corestorage: data schema version is %d.", dbv);
}

static void corestorage_h_luid(database_handle_t *db, const char *type)
{
	myentity_set_last_uid(db_sread_word(db));
}

static void corestorage_h_cf(database_handle_t *db, const char *type)
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

static void corestorage_h_mu(database_handle_t *db, const char *type)
{
	const char *uid = NULL;
	const char *name;
	const char *pass, *email, *language;
	time_t reg, login;
	const char *sflags;
	unsigned int flags = 0;
	myuser_t *mu;

	if (dbv >= 10)
		uid = db_sread_word(db);

	name = db_sread_word(db);

	if (myuser_find(name))
	{
		slog(LG_INFO, "db-h-mu: line %d: skipping duplicate account %s", db->line, name);
		return;
	}

	if (strict_mode && uid && myuser_find_uid(uid))
	{
		slog(LG_INFO, "db-h-mu: line %d: skipping account %s with duplicate UID %s", db->line, name, uid);
		return;
	}

	pass = db_sread_word(db);
	email = db_sread_word(db);
	reg = db_sread_time(db);
	login = db_sread_time(db);
	if (dbv >= 8) {
		sflags = db_sread_word(db);
		if (!gflags_fromstr(mu_flags, sflags, &flags))
			slog(LG_INFO, "db-h-mu: line %d: confused by flags: %s", db->line, sflags);
	} else {
		flags = db_sread_uint(db);
	}
	language = db_read_word(db);


	mu = myuser_add_id(uid, name, pass, email, flags);
	mu->registered = reg;
	mu->lastlogin = login;
	if (language)
		mu->language = language_add(language);
}

static void corestorage_h_me(database_handle_t *db, const char *type)
{
	const char *dest, *src, *text;
	time_t sent;
	unsigned int status;
	myuser_t *mu;
	mymemo_t *mz;

	dest = db_sread_word(db);
	src = db_sread_word(db);
	sent = db_sread_time(db);
	status = db_sread_int(db);
	text = db_sread_str(db);

	if (!(mu = myuser_find(dest)))
	{
		slog(LG_DEBUG, "db-h-me: line %d: memo for unknown account %s", db->line, dest);
		return;
	}

	mz = smalloc(sizeof *mz);
	mowgli_strlcpy(mz->sender, src, NICKLEN);
	mowgli_strlcpy(mz->text, text, MEMOLEN);
	mz->sent = sent;
	mz->status = status;

	if (!(mz->status & MEMO_READ))
		mu->memoct_new++;

	mowgli_node_add(mz, mowgli_node_create(), &mu->memos);
}

static void corestorage_h_mi(database_handle_t *db, const char *type)
{
	myuser_t *mu;
	const char *user, *target;

	user = db_sread_word(db);
	target = db_sread_word(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-mi: line %d: ignore for unknown account %s", db->line, user);
		return;
	}

	mowgli_node_add(sstrdup(target), mowgli_node_create(), &mu->memo_ignores);
}

static void corestorage_h_ac(database_handle_t *db, const char *type)
{
	myuser_t *mu;
	const char *user, *mask;

	user = db_sread_word(db);
	mask = db_sread_word(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-ac: line %d: access entry for unknown account %s", db->line, user);
		return;
	}

	myuser_access_add(mu, mask);
}

static void corestorage_h_mn(database_handle_t *db, const char *type)
{
	myuser_t *mu;
	mynick_t *mn;
	const char *user, *nick;
	time_t reg, seen;

	user = db_sread_word(db);
	nick = db_sread_word(db);
	reg = db_sread_time(db);
	seen = db_sread_time(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-mn: line %d: registered nick %s for unknown account %s", db->line, nick, user);
		return;
	}

	if (mynick_find(nick))
	{
		slog(LG_INFO, "db-h-mn: line %d: skipping duplicate nick %s for account %s", db->line, nick, user);
		return;
	}

	mn = mynick_add(mu, nick);
	mn->registered = reg;
	mn->lastseen = seen;
}

static void corestorage_h_mcfp(database_handle_t *db, const char *type)
{
	const char *user, *certfp;
	myuser_t *mu;

	user = db_sread_word(db);
	certfp = db_sread_word(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-mcfp: certfp %s for unknown account %s", certfp, user);
		return;
	}

	mycertfp_add(mu, certfp);
}

static void corestorage_h_su(database_handle_t *db, const char *type)
{
	slog(LG_INFO, "db-h-su: line %d: metadata change subscriptions have been dropped, ignoring", db->line);
}

static void corestorage_h_nam(database_handle_t *db, const char *type)
{
	const char *user = db_sread_word(db);
	myuser_name_add(user);
}

static void corestorage_h_so(database_handle_t *db, const char *type)
{
	const char *user, *class, *pass;
	unsigned int flags = 0;
	myuser_t *mu;
	const char *sflags;

	user = db_sread_word(db);
	class = db_sread_word(db);
	if (dbv >= 8)
	{
		sflags = db_sread_word(db);
		if (!gflags_fromstr(soper_flags, sflags, &flags))
			slog(LG_INFO, "db-h-so: line %d: confused by flags %s",
			     db->line, sflags);
	} else {
		flags = db_sread_int(db);
	}
	pass = db_read_word(db);
	if (pass != NULL && !*pass)
		pass = NULL;

	if (!(mu = myuser_find(user)))
	{
		slog(LG_INFO, "db-h-so: soper for nonexistent account %s", user);
		return;
	}

	soper_add(entity(mu)->name, class, flags & ~SOPER_CONF, pass);
}

static void corestorage_h_mc(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *name = db_sread_word(db);
	const char *key;
	const char *sflags;
	unsigned int flags = 0;

	mowgli_strlcpy(buf, name, sizeof buf);
	mychan_t *mc = mychan_add(buf);

	mc->registered = db_sread_time(db);
	mc->used = db_sread_time(db);
	if (dbv >= 8) {
		sflags = db_sread_word(db);
		if (!gflags_fromstr(mc_flags, sflags, &flags))
			slog(LG_INFO, "db-h-mc: line %d: confused by flags %s",
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

static char *
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

static void corestorage_h_md(database_handle_t *db, const char *type)
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
		return;
	}

	metadata_add(obj, prop, value);
	free(newvalue);
}

static void corestorage_h_mda(database_handle_t *db, const char *type)
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

static void corestorage_h_ca(database_handle_t *db, const char *type)
{
	const char *chan, *target;
	time_t tmod;
	unsigned int flags;
	mychan_t *mc;
	myentity_t *mt;
	myentity_t *setter;

	chan = db_sread_word(db);
	target = db_sread_word(db);
	flags = flags_to_bitmask(db_sread_word(db), 0);

	/* UNBAN self and akick exempt have been split to +e per github #75 */
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
		slog(LG_INFO, "db-h-ca: line %d: chanacs for nonexistent channel %s - exiting to avoid data loss", db->line, chan);
		slog(LG_INFO, "db-h-ca: line %d: if this depends on a specific module or feature; please make sure", db->line);
		slog(LG_INFO, "db-h-ca: line %d: that feature is enabled.", db->line);
		exit(EXIT_FAILURE);
	}

	if (mt == NULL && !validhostmask(target))
	{
		slog(LG_INFO, "db-h-ca: line %d: chanacs for nonexistent target %s - exiting to avoid data loss", db->line, target);
		slog(LG_INFO, "db-h-ca: line %d: if this depends on a specific module or feature; please make sure", db->line);
		slog(LG_INFO, "db-h-ca: line %d: that feature is enabled.", db->line);
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

static void corestorage_h_si(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *mask, *setby, *reason;
	time_t settime;
	svsignore_t *svsignore;

	mask = db_sread_word(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);
	mowgli_strlcpy(buf, reason, sizeof buf);

	strip(buf);
	svsignore = svsignore_add(mask, reason);
	svsignore->settime = settime;
	svsignore->setby = strdup(setby);
}

static void corestorage_h_kid(database_handle_t *db, const char *type)
{
	me.kline_id = db_sread_int(db);
}

static void corestorage_h_kl(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *user, *host, *reason, *setby;
	unsigned int id = 0;
	time_t settime;
	long duration;
	kline_t *k;

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

static void corestorage_h_xid(database_handle_t *db, const char *type)
{
	me.xline_id = db_sread_int(db);
}

static void corestorage_h_xl(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *realname, *reason, *setby;
	unsigned int id = 0;
	time_t settime;
	long duration;
	xline_t *x;

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

static void corestorage_h_qid(database_handle_t *db, const char *type)
{
	me.qline_id = db_sread_int(db);
}

static void corestorage_h_ql(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *mask, *reason, *setby;
	unsigned int id = 0;
	time_t settime;
	long duration;
	qline_t *q;

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

static void corestorage_ignore_row(database_handle_t *db, const char *type)
{
	return;
}

static void corestorage_db_load(const char *filename)
{
	database_handle_t *db;

	db = db_open(filename, DB_READ);
	if (db == NULL)
		return;

	db_parse(db);
	db_close(db);
}

static void corestorage_db_write(void *filename)
{
	database_handle_t *db;

	db = db_open(filename, DB_WRITE);

	corestorage_db_save(db);
	hook_call_db_write(db);

	db_close(db);
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &corestorage_db_load;
	db_save = &corestorage_db_write;

	db_register_type_handler("DBV", corestorage_h_dbv);
	db_register_type_handler("MDEP", corestorage_ignore_row);
	db_register_type_handler("LUID", corestorage_h_luid);
	db_register_type_handler("CF", corestorage_h_cf);
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
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
