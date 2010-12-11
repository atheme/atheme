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
	unsigned int dbv;

	unsigned int nmu;
	unsigned int nmc;
	unsigned int nca;
	unsigned int nkl;
	unsigned int nxl;
	unsigned int nql;
} opensex_t;

/* flatfile state */
unsigned int muout = 0, mcout = 0, caout = 0, kout = 0, xout = 0, qout = 0;

/* write atheme.db (core fields) */
static void
opensex_db_save(database_handle_t *db)
{
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
	mowgli_node_t *n, *tn, *tn2;
	mowgli_patricia_iteration_state_t state;
	myentity_iteration_state_t mestate;

	errno = 0;

	/* reset state */
	muout = 0, mcout = 0, caout = 0, kout = 0, xout = 0, qout = 0;

	/* write the database version */
	db_start_row(db, "DBV");
	db_write_int(db, 8);
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
		db_write_word(db, entity(mu)->name);
		db_write_word(db, mu->pass);
		db_write_word(db, mu->email);
		db_write_time(db, mu->registered);
		db_write_time(db, mu->lastlogin);
		db_write_word(db, flags);
		db_write_word(db, language_get_name(mu->language));
		db_commit_row(db);

		muout++;

		MOWGLI_ITER_FOREACH(tn, object(mu)->metadata.head)
		{
			metadata_t *md = (metadata_t *)tn->data;

			db_start_row(db, "MDU");
			db_write_word(db, entity(mu)->name);
			db_write_word(db, md->name);
			db_write_str(db, md->value);
			db_commit_row(db);
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
		mcout++;

		MOWGLI_ITER_FOREACH(tn, mc->chanacs.head)
		{
			ca = (chanacs_t *)tn->data;

			db_start_row(db, "CA");
			db_write_word(db, ca->mychan->name);
			db_write_word(db, ca->entity ? ca->entity->name : ca->host);
			db_write_word(db, bitmask_to_flags(ca->level));
			db_write_time(db, ca->tmodified);
			db_commit_row(db);

			MOWGLI_ITER_FOREACH(tn2, object(ca)->metadata.head)
			{
				char buf[BUFSIZE];
				metadata_t *md = (metadata_t *)tn2->data;

				snprintf(buf, BUFSIZE, "%s:%s", ca->mychan->name, (ca->entity) ? ca->entity->name : ca->host);

				db_start_row(db, "MDA");
				db_write_word(db, buf);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}

			caout++;
		}

		MOWGLI_ITER_FOREACH(tn, object(mc)->metadata.head)
		{
			metadata_t *md = (metadata_t *)tn->data;

			db_start_row(db, "MDC");
			db_write_word(db, mc->name);
			db_write_word(db, md->name);
			db_write_str(db, md->value);
			db_commit_row(db);
		}
	}

	/* Old names */
	MOWGLI_PATRICIA_FOREACH(mun, &state, oldnameslist)
	{
		db_start_row(db, "NAM");
		db_write_word(db, mun->name);
		db_commit_row(db);

		MOWGLI_ITER_FOREACH(tn, object(mun)->metadata.head)
		{
			metadata_t *md = (metadata_t *)tn->data;

			db_start_row(db, "MDN");
			db_write_word(db, mun->name);
			db_write_word(db, md->name);
			db_write_str(db, md->value);
			db_commit_row(db);
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
		db_write_word(db, k->user);
		db_write_word(db, k->host);
		db_write_uint(db, k->duration);
		db_write_time(db, k->settime);
		db_write_word(db, k->setby);
		db_write_str(db, k->reason);
		db_commit_row(db);

		kout++;
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
		db_write_word(db, x->realname);
		db_write_uint(db, x->duration);
		db_write_time(db, x->settime);
		db_write_word(db, x->setby);
		db_write_str(db, x->reason);
		db_commit_row(db);

		xout++;
	}

	db_start_row(db, "QID");
	db_write_uint(db, me.qline_id);
	db_commit_row(db);

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		/* QL <mask> <duration> <settime> <setby> <reason> */
		db_start_row(db, "QL");
		db_write_word(db, q->mask);
		db_write_uint(db, q->duration);
		db_write_time(db, q->settime);
		db_write_word(db, q->setby);
		db_write_str(db, q->reason);
		db_commit_row(db);

		qout++;
	}

	/* DE <muout> <mcout> <caout> <kout> <xout> <qout> */
	db_start_row(db, "DE");
	db_write_uint(db, muout);
	db_write_uint(db, mcout);
	db_write_uint(db, caout);
	db_write_uint(db, kout);
	db_write_uint(db, xout);
	db_write_uint(db, qout);
	db_commit_row(db);
}

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

static void opensex_h_unknown(database_handle_t *db, const char *type)
{
	slog(LG_ERROR, "db %s:%d: unknown directive '%s'", db->file, db->line, type);
	slog(LG_ERROR, "opensex_h_unknown: exiting to avoid data loss");
	exit(EXIT_FAILURE);
}

static void opensex_h_dbv(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	rs->dbv = db_sread_int(db);
	slog(LG_INFO, "opensex: data format version is %d.", rs->dbv);
}

static void opensex_h_cf(database_handle_t *db, const char *type)
{
	unsigned int their_ca_all;
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

static void opensex_h_mu(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	const char *name = db_sread_word(db);
	const char *pass, *email, *language;
	time_t reg, login;
	const char *sflags;
	unsigned int flags = 0;
	myuser_t *mu;

	if (myuser_find(name))
	{
		slog(LG_INFO, "db-h-mu: line %d: skipping duplicate account %s", db->line, name);
		return;
	}

	pass = db_sread_word(db);
	email = db_sread_word(db);
	reg = db_sread_time(db);
	login = db_sread_time(db);
	if (rs->dbv >= 8) {
		sflags = db_sread_word(db);
		if (!gflags_fromstr(mu_flags, sflags, &flags))
			slog(LG_INFO, "db-h-mu: line %d: confused by flags: %s", db->line, sflags);
	} else {
		flags = db_sread_uint(db);
	}
	language = db_read_word(db);


	mu = myuser_add(name, pass, email, flags);
	mu->registered = reg;
	mu->lastlogin = login;
	if (language)
		mu->language = language_add(language);
	rs->nmu++;
}

static void opensex_h_me(database_handle_t *db, const char *type)
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
	strlcpy(mz->sender, src, NICKLEN);
	strlcpy(mz->text, text, MEMOLEN);
	mz->sent = sent;
	mz->status = status;

	if (!(mz->status & MEMO_READ))
		mu->memoct_new++;

	mowgli_node_add(mz, mowgli_node_create(), &mu->memos);
}

static void opensex_h_mi(database_handle_t *db, const char *type)
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

static void opensex_h_ac(database_handle_t *db, const char *type)
{
	myuser_t *mu;
	const char *user, *mask;

	user = db_sread_word(db);
	mask = db_sread_str(db);

	mu = myuser_find(user);
	if (!mu)
	{
		slog(LG_DEBUG, "db-h-ac: line %d: access entry for unknown account %s", db->line, user);
		return;
	}

	myuser_access_add(mu, mask);
}

static void opensex_h_mn(database_handle_t *db, const char *type)
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

static void opensex_h_mcfp(database_handle_t *db, const char *type)
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

static void opensex_h_su(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *user, *sub_user, *tags, *tag;
	myuser_t *mu;
	metadata_subscription_t *ms;

	user = db_sread_word(db);
	sub_user = db_sread_word(db);
	tags = db_sread_word(db);

	if (!(mu = myuser_find(user)))
	{
		slog(LG_INFO, "dh-h-su: line %d: subscription to %s:%s for unknown account %s", db->line,
		     sub_user, tags, user);
		return;
	}

	ms = smalloc(sizeof *ms);
	ms->mu = mu;

	strlcpy(buf, tags, sizeof buf);
	tag = strtok(buf, ",");
	do
	{
		mowgli_node_add(sstrdup(tag), mowgli_node_create(), &ms->taglist);
	} while ((tag = strtok(NULL, ",")));

	mowgli_node_add(ms, mowgli_node_create(), &mu->subscriptions);
}

static void opensex_h_nam(database_handle_t *db, const char *type)
{
	const char *user = db_sread_word(db);
	myuser_name_add(user);
}

static void opensex_h_so(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	const char *user, *class, *pass;
	unsigned int flags = 0;
	myuser_t *mu;
	const char *sflags;

	user = db_sread_word(db);
	class = db_sread_word(db);
	if (rs->dbv >= 8)
	{
		sflags = db_sread_word(db);
		if (!gflags_fromstr(soper_flags, sflags, &flags))
			slog(LG_INFO, "db-h-so: line %d: confused by flags %s",
			     db->line, sflags);
	} else {
		flags = db_sread_int(db);
	}
	pass = db_read_word(db);

	if (!(mu = myuser_find(user)))
	{
		slog(LG_INFO, "db-h-so: soper for nonexistent account %s", user);
		return;
	}

	soper_add(entity(mu)->name, class, flags & ~SOPER_CONF, pass);
}

static void opensex_h_mc(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char buf[4096];
	const char *name = db_sread_word(db);
	const char *key;
	const char *sflags;
	unsigned int flags = 0;

	strlcpy(buf, name, sizeof buf);
	mychan_t *mc = mychan_add(buf);

	mc->registered = db_sread_time(db);
	mc->used = db_sread_time(db);
	if (rs->dbv >= 8) {
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
		strlcpy(buf, key, sizeof buf);
		strip(buf);
		if (buf[0] && buf[0] != ':' && !strchr(buf, ','))
			mc->mlock_key = sstrdup(buf);
	}

	rs->nmc++;
}

static void opensex_h_md(database_handle_t *db, const char *type)
{
	const char *name = db_sread_word(db);
	const char *prop = db_sread_word(db);
	const char *value = db_sread_str(db);
	void *obj = NULL;

	if (!strcmp(type, "MDU"))
	{
		obj = myuser_find(name);
	}
	else if (!strcmp(type, "MDC"))
	{
		obj = mychan_find(name);
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
}

static void opensex_h_ca(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	const char *chan, *target;
	time_t tmod;
	unsigned int flags;
	mychan_t *mc;
	myentity_t *mt;

	chan = db_sread_word(db);
	target = db_sread_word(db);
	flags = flags_to_bitmask(db_sread_word(db), 0);
	tmod = db_sread_time(db);

	mc = mychan_find(chan);
	mt = myentity_find(target);

	if (mc == NULL)
	{
		slog(LG_INFO, "db-h-ca: line %d: chanacs for nonexistent channel %s", db->line, chan);
		return;
	}

	if (mt == NULL && !validhostmask(target))
	{
		slog(LG_INFO, "db-h-ca: line %d: chanacs for nonexistent target %s", db->line, target);
		return;
	}

	if (mt == NULL && validhostmask(target))
	{
		chanacs_add_host(mc, target, flags, tmod);
	}
	else
	{
		chanacs_add(mc, mt, flags, tmod);
	}

	rs->nca++;
}

static void opensex_h_si(database_handle_t *db, const char *type)
{
	char buf[4096];
	const char *mask, *setby, *reason;
	time_t settime;
	svsignore_t *svsignore;

	mask = db_sread_word(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);
	strlcpy(buf, reason, sizeof buf);

	strip(buf);
	svsignore = svsignore_add(mask, reason);
	svsignore->settime = settime;
	svsignore->setby = strdup(setby);
}

static void opensex_h_kid(database_handle_t *db, const char *type)
{
	me.kline_id = db_sread_int(db);
}

static void opensex_h_kl(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char buf[4096];
	const char *user, *host, *reason, *setby;
	time_t settime;
	long duration;
	kline_t *k;

	user = db_sread_word(db);
	host = db_sread_word(db);
	duration = db_sread_uint(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);

	strlcpy(buf, reason, sizeof buf);
	strip(buf);

	k = kline_add(user, host, buf, duration, setby);
	k->settime = settime;
	k->expires = k->settime + k->duration;
	rs->nkl++;
}

static void opensex_h_xid(database_handle_t *db, const char *type)
{
	me.xline_id = db_sread_int(db);
}

static void opensex_h_xl(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char buf[4096];
	const char *realname, *reason, *setby;
	time_t settime;
	long duration;
	xline_t *x;

	realname = db_sread_word(db);
	duration = db_sread_uint(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);

	strlcpy(buf, reason, sizeof buf);
	strip(buf);

	x = xline_add(realname, buf, duration, setby);
	x->settime = settime;
	x->expires = x->settime + x->duration;
	rs->nxl++;
}

static void opensex_h_qid(database_handle_t *db, const char *type)
{
	me.qline_id = db_sread_int(db);
}

static void opensex_h_ql(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char buf[4096];
	const char *mask, *reason, *setby;
	time_t settime;
	long duration;
	qline_t *q;

	mask = db_sread_word(db);
	duration = db_sread_uint(db);
	settime = db_sread_time(db);
	setby = db_sread_word(db);
	reason = db_sread_str(db);

	strlcpy(buf, reason, sizeof buf);
	strip(buf);

	q = qline_add(mask, buf, duration, setby);
	q->settime = settime;
	q->expires = q->settime + q->duration;
	rs->nql++;
}

static void opensex_h_de(database_handle_t *db, const char *type)
{
	opensex_t *rs = (opensex_t *)db->priv;
	unsigned int nmu, nmc, nca, nkl, nxl, nql;

	nmu = db_sread_uint(db);
	nmc = db_sread_uint(db);
	nca = db_sread_uint(db);
	nkl = db_sread_uint(db);
	nxl = db_sread_uint(db);
	nql = db_sread_uint(db);

	if (nmu != rs->nmu)
		slog(LG_ERROR, "db-h-de: got %d myusers; expected %d", rs->nmu, nmu);
	if (nmc != rs->nmc)
		slog(LG_ERROR, "db-h-de: got %d mychans; expected %d", rs->nmc, nmc);
	if (nca != rs->nca)
		slog(LG_ERROR, "db-h-de: got %d chanacs; expected %d", rs->nca, nca);
	if (nkl != rs->nkl)
		slog(LG_ERROR, "db-h-de: got %d klines; expected %d", rs->nkl, nkl);
	if (nxl != rs->nxl)
		slog(LG_ERROR, "db-h-de: got %d xlines; expected %d", rs->nxl, nxl);
	if (nql != rs->nql)
		slog(LG_ERROR, "db-h-de: got %d qlines; expected %d", rs->nql, nql);
}

/***************************************************************************************************/

static bool opensex_read_next_row(database_handle_t *hdl)
{
	int c = 0;
	unsigned int n = 0;
	opensex_t *rs = (opensex_t *)hdl->priv;

	rs->token = NULL;

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
	char *res = strtok_r((db->token ? NULL : rs->buf), " ", &rs->token);
	db->token++;
	return res;
}

static const char *opensex_read_str(database_handle_t *db)
{
	opensex_t *rs = (opensex_t *)db->priv;
	char *res = strtok_r((db->token ? NULL : rs->buf), "", &rs->token);
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

	fprintf(rs->f, "%s ", type);

	return true;
}

static bool opensex_write_word(database_handle_t *db, const char *word)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	return_val_if_fail(word != NULL, false);
	rs = (opensex_t *)db->priv;

	fprintf(rs->f, "%s ", word);

	return true;
}

static bool opensex_write_str(database_handle_t *db, const char *word)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	return_val_if_fail(word != NULL, false);
	rs = (opensex_t *)db->priv;

	fprintf(rs->f, "%s", word);

	return true;
}

static bool opensex_write_int(database_handle_t *db, int num)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	rs = (opensex_t *)db->priv;

	fprintf(rs->f, "%d ", num);

	return true;
}

static bool opensex_write_uint(database_handle_t *db, unsigned int num)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	rs = (opensex_t *)db->priv;

	fprintf(rs->f, "%u ", num);

	return true;
}

static bool opensex_write_time(database_handle_t *db, time_t tm)
{
	opensex_t *rs;
	return_val_if_fail(db != NULL, false);
	rs = (opensex_t *)db->priv;

	fprintf(rs->f, "%lu ", (unsigned long)tm);

	return true;
}

static bool opensex_commit_row(database_handle_t *db)
{
	opensex_t *rs;

	return_val_if_fail(db != NULL, false);
	rs = (opensex_t *)db->priv;

	fprintf(rs->f, "\n");

	return true;
}

database_vtable_t opensex_vt = {
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

static database_handle_t *opensex_db_open_read(void)
{
	database_handle_t *db;
	opensex_t *rs;
	FILE *f;
	static const char *path = DATADIR "/services.db";
	int errno1;

	f = fopen(path, "r");
	if (!f)
	{
		errno1 = errno;
		slog(LG_ERROR, "db-open-read: cannot open '%s' for reading: %s", path, strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: db-open-read: cannot open '%s' for reading: %s"), path, strerror(errno1));
		return NULL;
	}

	rs = scalloc(sizeof(opensex_t), 1);
	rs->buf = scalloc(512, 1);
	rs->bufsize = 512;
	rs->token = NULL;
	rs->f = f;

	db = scalloc(sizeof(database_handle_t), 1);
	db->priv = rs;
	db->vt = &opensex_vt;
	db->txn = DB_READ;
	db->file = path;
	db->line = 0;
	db->token = 0;

	return db;
}

static database_handle_t *opensex_db_open_write(void)
{
	database_handle_t *db;
	opensex_t *rs;
	FILE *f;
	static const char *path = DATADIR "/services.db.new";
	int errno1;

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

	db = scalloc(sizeof(database_handle_t), 1);
	db->priv = rs;
	db->vt = &opensex_vt;
	db->txn = DB_WRITE;
	db->file = path;
	db->line = 0;
	db->token = 0;

	return db;
}

static database_handle_t *opensex_db_open(database_transaction_t txn)
{
	if (txn == DB_WRITE)
		return opensex_db_open_write();
	return opensex_db_open_read();
}

static void opensex_db_close(database_handle_t *db)
{
	opensex_t *rs;
	int errno1;

	return_if_fail(db != NULL);
	rs = db->priv;

	fclose(rs->f);

	if (db->txn == DB_WRITE)
	{
		/* now, replace the old database with the new one, using an atomic rename */
#ifdef _WIN32
		unlink(DATADIR "/services.db");
#endif
	
		if ((rename(DATADIR "/services.db.new", DATADIR "/services.db")) < 0)
		{
			errno1 = errno;
			slog(LG_ERROR, "db_save(): cannot rename services.db.new to services.db: %s", strerror(errno1));
			wallops(_("\2DATABASE ERROR\2: db_save(): cannot rename services.db.new to services.db: %s"), strerror(errno1));
		}

		hook_call_db_saved();
	}

	free(rs->buf);
	free(rs);
	free(db);
}

static void opensex_db_load(void)
{
	database_handle_t *db;

	db = db_open(DB_READ);
	opensex_db_parse(db);
	db_close(db);
}

static void opensex_db_write(void *arg)
{
	database_handle_t *db;

	db = db_open(DB_WRITE);

	opensex_db_save(db);
	hook_call_db_write(db);

	db_close(db);
}

database_module_t opensex_mod = {
	opensex_db_open,
	opensex_db_close
};

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_mod = &opensex_mod;
	db_load = &opensex_db_load;
	db_save = &opensex_db_write;

	db_register_type_handler("DBV", opensex_h_dbv);
	db_register_type_handler("CF", opensex_h_cf);
	db_register_type_handler("MU", opensex_h_mu);
	db_register_type_handler("ME", opensex_h_me);
	db_register_type_handler("MI", opensex_h_mi);
	db_register_type_handler("AC", opensex_h_ac);
	db_register_type_handler("MN", opensex_h_mn);
	db_register_type_handler("MCFP", opensex_h_mcfp);
	db_register_type_handler("SU", opensex_h_su);
	db_register_type_handler("NAM", opensex_h_nam);
	db_register_type_handler("SO", opensex_h_so);
	db_register_type_handler("MC", opensex_h_mc);
	db_register_type_handler("MDU", opensex_h_md);
	db_register_type_handler("MDC", opensex_h_md);
	db_register_type_handler("MDA", opensex_h_md);
	db_register_type_handler("MDN", opensex_h_md);
	db_register_type_handler("CA", opensex_h_ca);
	db_register_type_handler("SI", opensex_h_si);

	db_register_type_handler("KID", opensex_h_kid);
	db_register_type_handler("KL", opensex_h_kl);
	db_register_type_handler("XID", opensex_h_xid);
	db_register_type_handler("XL", opensex_h_xl);
	db_register_type_handler("QID", opensex_h_qid);
	db_register_type_handler("QL", opensex_h_ql);

	db_register_type_handler("DE", opensex_h_de);

	db_register_type_handler("???", opensex_h_unknown);

	backend_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
