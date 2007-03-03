/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains data structures, and functions to
 * manipulate them.
 *
 * $Id: node.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"
#include "uplink.h"

list_t klnlist;

static BlockHeap *kline_heap;	/* 16 */

/*************
 * L I S T S *
 *************/

void init_nodes(void)
{
	kline_heap = BlockHeapCreate(sizeof(kline_t), 16);

	if (kline_heap == NULL)
	{
		slog(LG_INFO, "init_nodes(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	init_uplinks();
	init_servers();
	init_accounts();
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

kline_t *kline_add(char *user, char *host, char *reason, long duration)
{
	kline_t *k;
	node_t *n = node_create();
	static uint32_t kcnt = 0;

	slog(LG_DEBUG, "kline_add(): %s@%s -> %s (%ld)", user, host, reason, duration);

	k = BlockHeapAlloc(kline_heap);

	node_add(k, n, &klnlist);

	k->user = sstrdup(user);
	k->host = sstrdup(host);
	k->reason = sstrdup(reason);
	k->duration = duration;
	k->settime = CURRTIME;
	k->expires = CURRTIME + duration;
	k->number = ++kcnt;

	cnt.kline++;

	kline_sts("*", user, host, duration, reason);

	return k;
}

void kline_delete(const char *user, const char *host)
{
	kline_t *k = kline_find(user, host);
	node_t *n;

	if (!k)
	{
		slog(LG_DEBUG, "kline_delete(): called for nonexistant kline: %s@%s", user, host);

		return;
	}

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

kline_t *kline_find_num(uint32_t number)
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
			snoop("KLINE:EXPIRE: \2%s@%s\2 set \2%s\2 ago by \2%s\2",
				k->user, k->host, time_ago(k->settime), k->setby);

			verbose_wallops("AKILL expired on \2%s@%s\2, set by \2%s\2",
				k->user, k->host, k->setby);

			kline_delete(k->user, k->host);
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
