/*
 * atheme-services: A collection of minimalist IRC services
 * node.c: Data structure management.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "uplink.h"
#include "privs.h"

list_t klnlist;
list_t xlnlist;
list_t qlnlist;

static BlockHeap *kline_heap;	/* 16 */
static BlockHeap *xline_heap;	/* 16 */
static BlockHeap *qline_heap;	/* 16 */

/*************
 * L I S T S *
 *************/

void init_nodes(void)
{
	kline_heap = BlockHeapCreate(sizeof(kline_t), 16);
	xline_heap = BlockHeapCreate(sizeof(xline_t), 16);
	qline_heap = BlockHeapCreate(sizeof(qline_t), 16);

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
void mark_all_illegal()
{
	node_t *n, *tn;
	uplink_t *u;
	soper_t *soper;
	operclass_t *operclass;

	LIST_FOREACH(n, uplinks.head)
	{
		u = (uplink_t *)n->data;
		u->flags |= UPF_ILLEGAL;
	}

	/* just delete these, we can survive without for a while */
	LIST_FOREACH_SAFE(n, tn, soperlist.head)
	{
		soper = (soper_t *)n->data;
		if (soper->flags & SOPER_CONF)
			soper_delete(soper);
	}
	/* no sopers pointing to these anymore */
	LIST_FOREACH_SAFE(n, tn, operclasslist.head)
	{
		operclass = (operclass_t *)n->data;
		operclass_delete(operclass);
	}
}

/* Unmark everything illegal, to be called after a failed rehash -- jilles */
void unmark_all_illegal()
{
	node_t *n;
	uplink_t *u;

	LIST_FOREACH(n, uplinks.head)
	{
		u = (uplink_t *)n->data;
		u->flags &= ~UPF_ILLEGAL;
	}
}

/* Remove illegal stuff, to be called after a successful rehash -- jilles */
void remove_illegals()
{
	node_t *n, *tn;
	uplink_t *u;

	LIST_FOREACH_SAFE(n, tn, uplinks.head)
	{
		u = (uplink_t *)n->data;
		if (u->flags & UPF_ILLEGAL && u != curr_uplink)
			uplink_delete(u);
	}
}

/*************
 * K L I N E *
 *************/

kline_t *kline_add(const char *user, const char *host, const char *reason, long duration, const char *setby)
{
	kline_t *k;
	node_t *n = node_create();

	slog(LG_DEBUG, "kline_add(): %s@%s -> %s (%ld)", user, host, reason, duration);

	k = BlockHeapAlloc(kline_heap);

	node_add(k, n, &klnlist);

	k->user = sstrdup(user);
	k->host = sstrdup(host);
	k->reason = sstrdup(reason);
	k->setby = sstrdup(setby);
	k->duration = duration;
	k->settime = CURRTIME;
	k->expires = CURRTIME + duration;
	k->number = ++me.kline_id;

	cnt.kline++;


	char treason[BUFSIZE];
	snprintf(treason, sizeof(treason), "[#%lu] %s", k->number, k->reason);

	kline_sts("*", user, host, duration, treason);

	return k;
}

void kline_delete(kline_t *k)
{
	node_t *n;

	return_if_fail(k != NULL);

	slog(LG_DEBUG, "kline_delete(): %s@%s -> %s", k->user, k->host, k->reason);
	/* only unkline if ircd has not already removed this -- jilles */
	if (k->duration == 0 || k->expires > CURRTIME)
		unkline_sts("*", k->user, k->host);

	n = node_find(k, &klnlist);
	node_del(n, &klnlist);
	node_free(n);

	free(k->user);
	free(k->host);
	free(k->reason);
	free(k->setby);

	BlockHeapFree(kline_heap, k);

	cnt.kline--;
}

kline_t *kline_find(const char *user, const char *host)
{
	kline_t *k;
	node_t *n;

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if ((!match(k->user, user)) && (!match(k->host, host)))
			return k;
	}

	return NULL;
}

kline_t *kline_find_num(unsigned long number)
{
	kline_t *k;
	node_t *n;

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->number == number)
			return k;
	}

	return NULL;
}

kline_t *kline_find_user(user_t *u)
{
	kline_t *k;
	node_t *n;

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->duration != 0 && k->expires <= CURRTIME)
			continue;
		if (!match(k->user, u->user) && (!match(k->host, u->host) || !match(k->host, u->ip) || !match_ips(k->host, u->ip)))
			return k;
	}

	return NULL;
}

void kline_expire(void *arg)
{
	kline_t *k;
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->duration == 0)
			continue;

		if (k->expires <= CURRTIME)
		{
			slog(LG_INFO, _("KLINE:EXPIRE: \2%s@%s\2 set \2%s\2 ago by \2%s\2"),
				k->user, k->host, time_ago(k->settime), k->setby);

			verbose_wallops(_("AKILL expired on \2%s@%s\2, set by \2%s\2"),
				k->user, k->host, k->setby);

			kline_delete(k);
		}
	}
}

/*************
 * X L I N E *
 *************/

xline_t *xline_add(const char *realname, const char *reason, long duration, const char *setby)
{
	xline_t *x;
	node_t *n = node_create();
	static unsigned int xcnt = 0;

	slog(LG_DEBUG, "xline_add(): %s -> %s (%ld)", realname, reason, duration);

	x = BlockHeapAlloc(xline_heap);

	node_add(x, n, &xlnlist);

	x->realname = sstrdup(realname);
	x->reason = sstrdup(reason);
	x->setby = sstrdup(setby);
	x->duration = duration;
	x->settime = CURRTIME;
	x->expires = CURRTIME + duration;
	x->number = ++xcnt;

	cnt.xline++;

	xline_sts("*", realname, duration, reason);

	return x;
}

void xline_delete(const char *realname)
{
	xline_t *x = xline_find(realname);
	node_t *n;

	if (!x)
	{
		slog(LG_DEBUG, "xline_delete(): called for nonexistant xline: %s", realname);
		return;
	}

	slog(LG_DEBUG, "xline_delete(): %s -> %s", x->realname, x->reason);

	/* only unxline if ircd has not already removed this -- jilles */
	if (x->duration == 0 || x->expires > CURRTIME)
		unxline_sts("*", x->realname);

	n = node_find(x, &xlnlist);
	node_del(n, &xlnlist);
	node_free(n);

	free(x->realname);
	free(x->reason);
	free(x->setby);

	BlockHeapFree(xline_heap, x);

	cnt.xline--;
}

xline_t *xline_find(const char *realname)
{
	xline_t *x;
	node_t *n;

	LIST_FOREACH(n, xlnlist.head)
	{
		x = (xline_t *)n->data;

		if (!match(x->realname, realname))
			return x;
	}

	return NULL;
}

xline_t *xline_find_num(unsigned int number)
{
	xline_t *x;
	node_t *n;

	LIST_FOREACH(n, xlnlist.head)
	{
		x = (xline_t *)n->data;

		if (x->number == number)
			return x;
	}

	return NULL;
}

xline_t *xline_find_user(user_t *u)
{
	xline_t *x;
	node_t *n;

	LIST_FOREACH(n, xlnlist.head)
	{
		x = (xline_t *)n->data;

		if (x->duration != 0 && x->expires <= CURRTIME)
			continue;

		if (!match(x->realname, u->gecos))
			return x;
	}

	return NULL;
}

void xline_expire(void *arg)
{
	xline_t *x;
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, xlnlist.head)
	{
		x = (xline_t *)n->data;

		if (x->duration == 0)
			continue;

		if (x->expires <= CURRTIME)
		{
			slog(LG_INFO, _("XLINE:EXPIRE: \2%s\2 set \2%s\2 ago by \2%s\2"),
				x->realname, time_ago(x->settime), x->setby);

			verbose_wallops(_("XLINE expired on \2%s\2, set by \2%s\2"),
				x->realname, x->setby);

			xline_delete(x->realname);
		}
	}
}

/*************
 * Q L I N E *
 *************/

qline_t *qline_add(const char *mask, const char *reason, long duration, const char *setby)
{
	qline_t *q;
	node_t *n = node_create();
	static unsigned int qcnt = 0;

	slog(LG_DEBUG, "qline_add(): %s -> %s (%ld)", mask, reason, duration);

	q = BlockHeapAlloc(qline_heap);
	node_add(q, n, &qlnlist);

	q->mask = sstrdup(mask);
	q->reason = sstrdup(reason);
	q->setby = sstrdup(setby);
	q->duration = duration;
	q->settime = CURRTIME;
	q->expires = CURRTIME + duration;
	q->number = ++qcnt;

	cnt.qline++;

	qline_sts("*", mask, duration, reason);

	return q;
}

void qline_delete(const char *mask)
{
	qline_t *q = qline_find(mask);
	node_t *n;

	if (!q)
	{
		slog(LG_DEBUG, "qline_delete(): called for nonexistant qline: %s", mask);
		return;
	}

	slog(LG_DEBUG, "qline_delete(): %s -> %s", q->mask, q->reason);

	/* only unqline if ircd has not already removed this -- jilles */
	if (q->duration == 0 || q->expires > CURRTIME)
		unqline_sts("*", q->mask);

	n = node_find(q, &qlnlist);
	node_del(n, &qlnlist);
	node_free(n);

	free(q->mask);
	free(q->reason);
	free(q->setby);

	BlockHeapFree(qline_heap, q);

	cnt.qline--;
}

qline_t *qline_find(const char *mask)
{
	qline_t *q;
	node_t *n;
	bool ischan;

	LIST_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		ischan = q->mask[0] == '#' || q->mask[0] == '&';
		if (!ischan && !match(q->mask, mask))
			return q;
		else if (ischan && !irccasecmp(q->mask, mask))
			return q;
	}

	return NULL;
}

qline_t *qline_find_num(unsigned int number)
{
	qline_t *q;
	node_t *n;

	LIST_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		if (q->number == number)
			return q;
	}

	return NULL;
}

qline_t *qline_find_user(user_t *u)
{
	qline_t *q;
	node_t *n;

	LIST_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		if (q->duration != 0 && q->expires <= CURRTIME)
			continue;
		if (q->mask[0] == '#' || q->mask[0] == '&')
			continue;
		if (!match(q->mask, u->nick))
			return q;
	}

	return NULL;
}

qline_t *qline_find_channel(channel_t *c)
{
	qline_t *q;
	node_t *n;

	LIST_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		if (q->duration != 0 && q->expires <= CURRTIME)
			continue;
		if (!irccasecmp(q->mask, c->name))
			return q;
	}

	return NULL;
}

void qline_expire(void *arg)
{
	qline_t *q;
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, qlnlist.head)
	{
		q = (qline_t *)n->data;

		if (q->duration == 0)
			continue;

		if (q->expires <= CURRTIME)
		{
			slog(LG_INFO, _("QLINE:EXPIRE: \2%s\2 set \2%s\2 ago by \2%s\2"),
				q->mask, time_ago(q->settime), q->setby);

			verbose_wallops(_("QLINE expired on \2%s\2, set by \2%s\2"),
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
