/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2015-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * node.c: Data structure management.
 */

#include <atheme.h>
#include "internal.h"

mowgli_list_t klnlist;
mowgli_list_t xlnlist;
mowgli_list_t qlnlist;

static mowgli_heap_t *kline_heap = NULL;	/* 16 */
static mowgli_heap_t *xline_heap = NULL;	/* 16 */
static mowgli_heap_t *qline_heap = NULL;	/* 16 */

/*************
 * L I S T S *
 *************/

void
init_nodes(void)
{
	kline_heap = sharedheap_get(sizeof(struct kline));
	xline_heap = sharedheap_get(sizeof(struct xline));
	qline_heap = sharedheap_get(sizeof(struct qline));

	if (kline_heap == NULL || xline_heap == NULL || qline_heap == NULL)
	{
		slog(LG_INFO, "init_nodes(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	init_uplinks();
	init_servers();
	init_metadata();
	init_accounts();
	init_entities();
	init_users();
	init_channels();
	init_privs();
}

/* Mark everything illegal, to be called before a rehash -- jilles */
void
mark_all_illegal()
{
	mowgli_node_t *n, *tn;
	struct uplink *u;
	struct soper *soper;
	struct operclass *operclass;

	MOWGLI_ITER_FOREACH(n, uplinks.head)
	{
		u = (struct uplink *)n->data;
		u->flags |= UPF_ILLEGAL;
	}

	/* just delete these, we can survive without for a while */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, soperlist.head)
	{
		soper = (struct soper *)n->data;
		if (soper->flags & SOPER_CONF)
			soper_delete(soper);
	}
	/* no sopers pointing to these anymore */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, operclasslist.head)
	{
		operclass = (struct operclass *)n->data;
		operclass_delete(operclass);
	}
}

/* Unmark everything illegal, to be called after a failed rehash -- jilles */
void
unmark_all_illegal()
{
	mowgli_node_t *n;
	struct uplink *u;

	MOWGLI_ITER_FOREACH(n, uplinks.head)
	{
		u = (struct uplink *)n->data;
		u->flags &= ~UPF_ILLEGAL;
	}
}

/* Remove illegal stuff, to be called after a successful rehash -- jilles */
void
remove_illegals()
{
	mowgli_node_t *n, *tn;
	struct uplink *u;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, uplinks.head)
	{
		u = (struct uplink *)n->data;
		if (u->flags & UPF_ILLEGAL && u != curr_uplink)
			uplink_delete(u);
	}
}

/*************
 * K L I N E *
 *************/

struct kline *
kline_add_with_id(const char *user, const char *host, const char *reason, long duration, const char *setby, unsigned long id)
{
	struct kline *k;
	mowgli_node_t *n = mowgli_node_create();

	slog(LG_DEBUG, "kline_add(): %s@%s -> %s (%ld)", user, host, reason, duration);

	k = mowgli_heap_alloc(kline_heap);

	mowgli_node_add(k, n, &klnlist);

	k->user = sstrdup(user);
	k->host = sstrdup(host);
	k->reason = sstrdup(reason);
	k->setby = sstrdup(setby);
	k->duration = duration;
	k->settime = CURRTIME;
	k->expires = CURRTIME + duration;
	k->number = id;

	cnt.kline++;


	char treason[BUFSIZE];
	snprintf(treason, sizeof(treason), "[#%lu] %s", k->number, k->reason);

	if (me.connected)
		kline_sts("*", user, host, duration, treason);

	return k;
}

struct kline *
kline_add(const char *user, const char *host, const char *reason, long duration, const char *setby)
{
	return kline_add_with_id(user, host, reason, duration, setby, ++me.kline_id);
}

struct kline *
kline_add_user(struct user *u, const char *reason, long duration, const char *setby)
{
	bool use_ident = config_options.kline_with_ident && (!config_options.kline_verified_ident || *u->user != '~');

	return kline_add (use_ident ? u->user : "*", u->ip ? u->ip : u->host, reason, duration, setby);
}

void
kline_delete(struct kline *k)
{
	mowgli_node_t *n;

	return_if_fail(k != NULL);

	slog(LG_DEBUG, "kline_delete(): %s@%s -> %s", k->user, k->host, k->reason);

	/* only unkline if ircd has not already removed this -- jilles */
	if (me.connected && (k->duration == 0 || k->expires > CURRTIME))
		unkline_sts("*", k->user, k->host);

	n = mowgli_node_find(k, &klnlist);
	mowgli_node_delete(n, &klnlist);
	mowgli_node_free(n);

	sfree(k->user);
	sfree(k->host);
	sfree(k->reason);
	sfree(k->setby);

	mowgli_heap_free(kline_heap, k);

	cnt.kline--;
}

struct kline *
kline_find(const char *user, const char *host)
{
	struct kline *k;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, klnlist.head)
	{
		k = (struct kline *)n->data;

		if ((!match(k->user, user)) && (!match(k->host, host)))
			return k;
	}

	return NULL;
}

struct kline *
kline_find_num(unsigned long number)
{
	struct kline *k;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, klnlist.head)
	{
		k = (struct kline *)n->data;

		if (k->number == number)
			return k;
	}

	return NULL;
}

struct kline *
kline_find_user(struct user *u)
{
	struct kline *k;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, klnlist.head)
	{
		k = (struct kline *)n->data;

		if (k->duration != 0 && k->expires <= CURRTIME)
			continue;
		if (!match(k->user, u->user) && (!match(k->host, u->host) || !match(k->host, u->ip) || !match_ips(k->host, u->ip)))
			return k;
	}

	return NULL;
}

void
kline_expire(void *arg)
{
	struct kline *k;
	char *reason;
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, klnlist.head)
	{
		k = (struct kline *)n->data;

		if (k->duration == 0)
			continue;

		if (k->expires <= CURRTIME)
		{
			/* TODO: determine validity of k->reason */
			reason = k->reason ? k->reason : "(none)";

			slog(LG_INFO, "KLINE:EXPIRE: \2%s@%s\2 set \2%s\2 ago by \2%s\2 (reason: %s)",
				k->user, k->host, time_ago(k->settime), k->setby, reason);

			verbose_wallops("AKILL expired on \2%s@%s\2, set by \2%s\2 (reason: %s)",
				k->user, k->host, k->setby, reason);

			kline_delete(k);
		}
	}
}

/*************
 * X L I N E *
 *************/

struct xline *
xline_add(const char *realname, const char *reason, long duration, const char *setby)
{
	struct xline *x;
	mowgli_node_t *n = mowgli_node_create();
	static unsigned int xcnt = 0;

	slog(LG_DEBUG, "xline_add(): %s -> %s (%ld)", realname, reason, duration);

	x = mowgli_heap_alloc(xline_heap);

	mowgli_node_add(x, n, &xlnlist);

	x->realname = sstrdup(realname);
	x->reason = sstrdup(reason);
	x->setby = sstrdup(setby);
	x->duration = duration;
	x->settime = CURRTIME;
	x->expires = CURRTIME + duration;
	x->number = ++xcnt;

	cnt.xline++;

	if (me.connected)
		xline_sts("*", realname, duration, reason);

	return x;
}

void
xline_delete(const char *realname)
{
	struct xline *x = xline_find(realname);
	mowgli_node_t *n;

	if (!x)
	{
		slog(LG_DEBUG, "xline_delete(): called for nonexistent xline: %s", realname);
		return;
	}

	slog(LG_DEBUG, "xline_delete(): %s -> %s", x->realname, x->reason);

	/* only unxline if ircd has not already removed this -- jilles */
	if (me.connected && (x->duration == 0 || x->expires > CURRTIME))
		unxline_sts("*", x->realname);

	n = mowgli_node_find(x, &xlnlist);
	mowgli_node_delete(n, &xlnlist);
	mowgli_node_free(n);

	sfree(x->realname);
	sfree(x->reason);
	sfree(x->setby);

	mowgli_heap_free(xline_heap, x);

	cnt.xline--;
}

struct xline *
xline_find(const char *realname)
{
	struct xline *x;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, xlnlist.head)
	{
		x = (struct xline *)n->data;

		if (!match(x->realname, realname))
			return x;
	}

	return NULL;
}

struct xline *
xline_find_num(unsigned int number)
{
	struct xline *x;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, xlnlist.head)
	{
		x = (struct xline *)n->data;

		if (x->number == number)
			return x;
	}

	return NULL;
}

struct xline *
xline_find_user(struct user *u)
{
	struct xline *x;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, xlnlist.head)
	{
		x = (struct xline *)n->data;

		if (x->duration != 0 && x->expires <= CURRTIME)
			continue;

		if (!match(x->realname, u->gecos))
			return x;
	}

	return NULL;
}

void
xline_expire(void *arg)
{
	struct xline *x;
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, xlnlist.head)
	{
		x = (struct xline *)n->data;

		if (x->duration == 0)
			continue;

		if (x->expires <= CURRTIME)
		{
			slog(LG_INFO, "XLINE:EXPIRE: \2%s\2 set \2%s\2 ago by \2%s\2",
				x->realname, time_ago(x->settime), x->setby);

			verbose_wallops("XLINE expired on \2%s\2, set by \2%s\2",
				x->realname, x->setby);

			xline_delete(x->realname);
		}
	}
}

/*************
 * Q L I N E *
 *************/

struct qline *
qline_add(const char *mask, const char *reason, long duration, const char *setby)
{
	struct qline *q;
	mowgli_node_t *n = mowgli_node_create();
	static unsigned int qcnt = 0;

	slog(LG_DEBUG, "qline_add(): %s -> %s (%ld)", mask, reason, duration);

	q = mowgli_heap_alloc(qline_heap);
	mowgli_node_add(q, n, &qlnlist);

	q->mask = sstrdup(mask);
	q->reason = sstrdup(reason);
	q->setby = sstrdup(setby);
	q->duration = duration;
	q->settime = CURRTIME;
	q->expires = CURRTIME + duration;
	q->number = ++qcnt;

	cnt.qline++;

	if (me.connected)
		qline_sts("*", mask, duration, reason);

	return q;
}

void
qline_delete(const char *mask)
{
	struct qline *q = qline_find(mask);
	mowgli_node_t *n;

	if (!q)
	{
		slog(LG_DEBUG, "qline_delete(): called for nonexistent qline: %s", mask);
		return;
	}

	slog(LG_DEBUG, "qline_delete(): %s -> %s", q->mask, q->reason);

	/* only unqline if ircd has not already removed this -- jilles */
	if (me.connected && (q->duration == 0 || q->expires > CURRTIME))
		unqline_sts("*", q->mask);

	n = mowgli_node_find(q, &qlnlist);
	mowgli_node_delete(n, &qlnlist);
	mowgli_node_free(n);

	sfree(q->mask);
	sfree(q->reason);
	sfree(q->setby);

	mowgli_heap_free(qline_heap, q);

	cnt.qline--;
}

struct qline *
qline_find(const char *mask)
{
	struct qline *q;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (!irccasecmp(q->mask, mask))
			return q;
	}

	return NULL;
}

struct qline *
qline_find_match(const char *mask)
{
	struct qline *q;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->duration != 0 && q->expires <= CURRTIME)
			continue;
		if (!match(q->mask, mask))
			return q;
	}

	return NULL;
}

struct qline *
qline_find_num(unsigned int number)
{
	struct qline *q;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->number == number)
			return q;
	}

	return NULL;
}

struct qline *
qline_find_user(struct user *u)
{
	struct qline *q;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->duration != 0 && q->expires <= CURRTIME)
			continue;
		if (q->mask[0] == '#' || q->mask[0] == '&')
			continue;
		if (!match(q->mask, u->nick))
			return q;
	}

	return NULL;
}

struct qline *
qline_find_channel(struct channel *c)
{
	struct qline *q;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->duration != 0 && q->expires <= CURRTIME)
			continue;
		if (!irccasecmp(q->mask, c->name))
			return q;
	}

	return NULL;
}

void
qline_expire(void *arg)
{
	struct qline *q;
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, qlnlist.head)
	{
		q = (struct qline *)n->data;

		if (q->duration == 0)
			continue;

		if (q->expires <= CURRTIME)
		{
			slog(LG_INFO, "QLINE:EXPIRE: \2%s\2 set \2%s\2 ago by \2%s\2",
				q->mask, time_ago(q->settime), q->setby);

			verbose_wallops("QLINE expired on \2%s\2, set by \2%s\2",
				q->mask, q->setby);

			qline_delete(q->mask);
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
