/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains data structures, and functions to
 * manipulate them.
 *
 * $Id: node.c 6895 2006-10-22 21:07:24Z jilles $
 */

#include "atheme.h"
#include "uplink.h"

list_t operclasslist;
list_t soperlist;
list_t uplinks;
list_t klnlist;
dictionary_tree_t *mclist;

static BlockHeap *operclass_heap;
static BlockHeap *soper_heap;
static BlockHeap *uplink_heap;

static BlockHeap *kline_heap;	/* 16 */
static BlockHeap *mychan_heap;	/* HEAP_CHANNEL */
static BlockHeap *chanacs_heap;	/* HEAP_CHANACS */
static BlockHeap *metadata_heap;	/* HEAP_CHANUSER */

boolean_t mychan_isused(mychan_t *mc);
myuser_t *mychan_pick_candidate(mychan_t *mc, uint32_t minlevel, int maxtime);
myuser_t *mychan_pick_successor(mychan_t *mc);

/*************
 * L I S T S *
 *************/

void init_nodes(void)
{
	operclass_heap = BlockHeapCreate(sizeof(operclass_t), 2);
	soper_heap = BlockHeapCreate(sizeof(soper_t), 2);
	uplink_heap = BlockHeapCreate(sizeof(uplink_t), 4);
	metadata_heap = BlockHeapCreate(sizeof(metadata_t), HEAP_CHANUSER);
	kline_heap = BlockHeapCreate(sizeof(kline_t), 16);
	mychan_heap = BlockHeapCreate(sizeof(mychan_t), HEAP_CHANNEL);
	chanacs_heap = BlockHeapCreate(sizeof(chanacs_t), HEAP_CHANUSER);

	if (!soper_heap || !uplink_heap || !metadata_heap || !kline_heap || !mychan_heap || !chanacs_heap)
	{
		slog(LG_INFO, "init_nodes(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	mclist = dictionary_create("mychan", HASH_CHANNEL, irccasecmp);

	init_servers();
	init_accounts();
	init_users();
	init_channels();
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

/*****************
 * U P L I N K S *
 *****************/

uplink_t *uplink_add(char *name, char *host, char *password, char *vhost, int port)
{
	uplink_t *u;
	node_t *n;

	slog(LG_DEBUG, "uplink_add(): %s -> %s:%d", me.name, name, port);

	if ((u = uplink_find(name)))
	{
		if (u->flags & UPF_ILLEGAL)
		{
			u->flags &= ~UPF_ILLEGAL;
			free(u->name);
			free(u->host);
			free(u->pass);
			free(u->vhost);
		}
		else
		{
			slog(LG_INFO, "Duplicate uplink %s.", name);
			return NULL;
		}
	}
	else
	{
		u = BlockHeapAlloc(uplink_heap);
		n = node_create();
		u->node = n;
		node_add(u, n, &uplinks);
		cnt.uplink++;
	}

	u->name = sstrdup(name);
	u->host = sstrdup(host);
	u->pass = sstrdup(password);
	if (vhost)
		u->vhost = sstrdup(vhost);
	else
		u->vhost = sstrdup("0.0.0.0");
	u->port = port;

	return u;
}

void uplink_delete(uplink_t * u)
{
	node_t *n = node_find(u, &uplinks);

	free(u->name);
	free(u->host);
	free(u->pass);
	free(u->vhost);

	node_del(n, &uplinks);
	node_free(n);

	BlockHeapFree(uplink_heap, u);
	cnt.uplink--;
}

uplink_t *uplink_find(char *name)
{
	node_t *n;

	LIST_FOREACH(n, uplinks.head)
	{
		uplink_t *u = n->data;

		if (!strcasecmp(u->name, name))
			return u;
	}

	return NULL;
}

/*************************
 * O P E R C L A S S E S *
 *************************/
operclass_t *operclass_add(char *name, char *privs)
{
	operclass_t *operclass;
	node_t *n = node_create();

	operclass = operclass_find(name);
	if (operclass != NULL)
	{
		slog(LG_DEBUG, "operclass_add(): duplicate class %s", name);
		return NULL;
	}
	slog(LG_DEBUG, "operclass_add(): %s [%s]", name, privs);
	operclass = BlockHeapAlloc(operclass_heap);
	node_add(operclass, n, &operclasslist);
	operclass->name = sstrdup(name);
	operclass->privs = sstrdup(privs);
	cnt.operclass++;
	return operclass;
}

void operclass_delete(operclass_t *operclass)
{
	node_t *n;

	if (operclass == NULL)
		return;
	slog(LG_DEBUG, "operclass_delete(): %s", operclass->name);
	n = node_find(operclass, &operclasslist);
	node_del(n, &operclasslist);
	node_free(n);
	free(operclass->name);
	free(operclass->privs);
	BlockHeapFree(operclass_heap, operclass);
	cnt.operclass--;
}

operclass_t *operclass_find(char *name)
{
	operclass_t *operclass;
	node_t *n;

	LIST_FOREACH(n, operclasslist.head)
	{
		operclass = (operclass_t *)n->data;

		if (!strcasecmp(operclass->name, name))
			return operclass;
	}

	return NULL;
}

/***************
 * S O P E R S *
 ***************/

soper_t *soper_add(char *name, operclass_t *operclass)
{
	soper_t *soper;
	myuser_t *mu = myuser_find(name);
	node_t *n;

	if (mu ? soper_find(mu) : soper_find_named(name))
	{
		slog(LG_INFO, "soper_add(): duplicate soper %s", name);
		return NULL;
	}
	slog(LG_DEBUG, "soper_add(): %s -> %s", (mu) ? mu->name : name, operclass ? operclass->name : "<null>");

	soper = BlockHeapAlloc(soper_heap);
	n = node_create();

	node_add(soper, n, &soperlist);

	if (mu)
	{
		soper->myuser = mu;
		mu->soper = soper;
		soper->name = NULL;
	}
	else
	{
		soper->name = sstrdup(name);
		soper->myuser = NULL;
	}
	soper->operclass = operclass;

	cnt.soper++;

	return soper;
}

void soper_delete(soper_t *soper)
{
	node_t *n;

	if (!soper)
	{
		slog(LG_DEBUG, "soper_delete(): called for null soper");

		return;
	}

	slog(LG_DEBUG, "soper_delete(): %s", (soper->myuser) ? soper->myuser->name : soper->name);

	n = node_find(soper, &soperlist);
	node_del(n, &soperlist);
	node_free(n);

	if (soper->myuser)
		soper->myuser->soper = NULL;

	if (soper->name)
		free(soper->name);

	BlockHeapFree(soper_heap, soper);

	cnt.soper--;
}

soper_t *soper_find(myuser_t *myuser)
{
	soper_t *soper;
	node_t *n;

	LIST_FOREACH(n, soperlist.head)
	{
		soper = (soper_t *)n->data;

		if (soper->myuser && soper->myuser == myuser)
			return soper;
	}

	return NULL;
}

soper_t *soper_find_named(char *name)
{
	soper_t *soper;
	node_t *n;

	LIST_FOREACH(n, soperlist.head)
	{
		soper = (soper_t *)n->data;

		if (soper->name && !irccasecmp(soper->name, name))
			return soper;
	}

	return NULL;
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

/***************
 * M Y C H A N *
 ***************/

mychan_t *mychan_add(char *name)
{
	mychan_t *mc;

	mc = mychan_find(name);

	if (mc)
	{
		slog(LG_DEBUG, "mychan_add(): mychan already exists: %s", name);
		return mc;
	}

	slog(LG_DEBUG, "mychan_add(): %s", name);

	mc = BlockHeapAlloc(mychan_heap);

	strlcpy(mc->name, name, CHANNELLEN);
	mc->founder = NULL;
	mc->registered = CURRTIME;
	mc->chan = channel_find(name);

	dictionary_add(mclist, mc->name, mc);

	cnt.mychan++;

	return mc;
}

void mychan_delete(char *name)
{
	mychan_t *mc = mychan_find(name);
	chanacs_t *ca;
	node_t *n, *tn;

	if (!mc)
	{
		slog(LG_DEBUG, "mychan_delete(): called for nonexistant mychan: %s", name);
		return;
	}

	slog(LG_DEBUG, "mychan_delete(): %s", mc->name);

	/* remove the chanacs shiz */
	LIST_FOREACH_SAFE(n, tn, mc->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->myuser)
			chanacs_delete(ca->mychan, ca->myuser, ca->level);
		else
			chanacs_delete_host(ca->mychan, ca->host, ca->level);
	}

	dictionary_delete(mclist, mc->name);

	BlockHeapFree(mychan_heap, mc);

	cnt.mychan--;
}

mychan_t *mychan_find(const char *name)
{
	return dictionary_retrieve(mclist, name);
}

/* Check if there is anyone on the channel fulfilling the conditions.
 * Fairly expensive, but this is sometimes necessary to avoid
 * inappropriate drops. -- jilles */
boolean_t mychan_isused(mychan_t *mc)
{
	node_t *n;
	channel_t *c;
	chanuser_t *cu;

	c = mc->chan;
	if (c == NULL)
		return FALSE;
	LIST_FOREACH(n, c->members.head)
	{
		cu = n->data;
		if (chanacs_user_flags(mc, cu->user) & CA_USEDUPDATE)
			return TRUE;
	}
	return FALSE;
}

/* Find a user fulfilling the conditions who can take another channel */
myuser_t *mychan_pick_candidate(mychan_t *mc, uint32_t minlevel, int maxtime)
{
	int j, tcnt;
	node_t *n, *n2;
	chanacs_t *ca;
	mychan_t *tmc;
	myuser_t *mu;
	dictionary_iteration_state_t state;

	LIST_FOREACH(n, mc->chanacs.head)
	{
		ca = n->data;
		if (ca->level & CA_AKICK)
			continue;
		mu = ca->myuser;
		if (mu == NULL || mu == mc->founder)
			continue;
		if ((ca->level & minlevel) == minlevel && (maxtime == 0 || LIST_LENGTH(&mu->logins) > 0 || CURRTIME - mu->lastlogin < maxtime))
		{
			if (has_priv_myuser(mu, PRIV_REG_NOLIMIT))
				return mu;
			tcnt = 0;
			DICTIONARY_FOREACH(tmc, &state, mclist)
			{
				if (is_founder(tmc, mu))
					tcnt++;
			}
		
			if (tcnt < me.maxchans)
				return mu;
		}
	}
	return NULL;
}

/* Pick a suitable successor
 * Note: please do not make this dependent on currently being in
 * the channel or on IRC; this would give an unfair advantage to
 * 24*7 clients and bots.
 * -- jilles */
myuser_t *mychan_pick_successor(mychan_t *mc)
{
	myuser_t *mu;

	/* full privs? */
	mu = mychan_pick_candidate(mc, CA_FOUNDER_0, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_FOUNDER_0, 0);
	if (mu != NULL)
		return mu;
	/* someone with +R then? (old successor has this, but not sop) */
	mu = mychan_pick_candidate(mc, CA_RECOVER, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_RECOVER, 0);
	if (mu != NULL)
		return mu;
	/* an op perhaps? */
	mu = mychan_pick_candidate(mc, CA_OP, 7*86400);
	if (mu != NULL)
		return mu;
	mu = mychan_pick_candidate(mc, CA_OP, 0);
	if (mu != NULL)
		return mu;
	/* just an active user with access */
	mu = mychan_pick_candidate(mc, 0, 7*86400);
	if (mu != NULL)
		return mu;
	/* ok you can't say we didn't try */
	return mychan_pick_candidate(mc, 0, 0);
}

/*****************
 * C H A N A C S *
 *****************/

chanacs_t *chanacs_add(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	chanacs_t *ca;
	node_t *n1;
	node_t *n2;

	if (!mychan || !myuser)
	{
		slog(LG_DEBUG, "chanacs_add(): got mychan == NULL or myuser == NULL, ignoring");
		return NULL;
	}

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add(): got non #channel: %s", mychan->name);
		return NULL;
	}

	slog(LG_DEBUG, "chanacs_add(): %s -> %s", mychan->name, myuser->name);

	n1 = node_create();
	n2 = node_create();

	ca = BlockHeapAlloc(chanacs_heap);

	ca->mychan = mychan;
	ca->myuser = myuser;
	ca->level = level & CA_ALL;

	node_add(ca, n1, &mychan->chanacs);
	node_add(ca, n2, &myuser->chanacs);

	cnt.chanacs++;

	return ca;
}

chanacs_t *chanacs_add_host(mychan_t *mychan, char *host, uint32_t level)
{
	chanacs_t *ca;
	node_t *n;

	if (!mychan || !host)
	{
		slog(LG_DEBUG, "chanacs_add_host(): got mychan == NULL or host == NULL, ignoring");
		return NULL;
	}

	if (*mychan->name != '#')
	{
		slog(LG_DEBUG, "chanacs_add_host(): got non #channel: %s", mychan->name);
		return NULL;
	}

	slog(LG_DEBUG, "chanacs_add_host(): %s -> %s", mychan->name, host);

	n = node_create();

	ca = BlockHeapAlloc(chanacs_heap);

	ca->mychan = mychan;
	ca->myuser = NULL;
	strlcpy(ca->host, host, HOSTLEN);
	ca->level |= level;

	node_add(ca, n, &mychan->chanacs);

	cnt.chanacs++;

	return ca;
}

void chanacs_delete(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	chanacs_t *ca;
	node_t *n, *tn, *n2;

	if (!mychan || !myuser)
	{
		slog(LG_DEBUG, "chanacs_delete(): got mychan == NULL or myuser == NULL, ignoring");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if ((ca->myuser == myuser) && (ca->level == level))
		{
			slog(LG_DEBUG, "chanacs_delete(): %s -> %s", ca->mychan->name, ca->myuser->name);
			node_del(n, &mychan->chanacs);
			node_free(n);

			n2 = node_find(ca, &myuser->chanacs);
			node_del(n2, &myuser->chanacs);
			node_free(n2);

			cnt.chanacs--;

			return;
		}
	}
}

void chanacs_delete_host(mychan_t *mychan, char *host, uint32_t level)
{
	chanacs_t *ca;
	node_t *n, *tn;

	if (!mychan || !host)
	{
		slog(LG_DEBUG, "chanacs_delete_host(): got mychan == NULL or myuser == NULL, ignoring");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if ((ca->myuser == NULL) && (!irccasecmp(host, ca->host)) && (ca->level == level))
		{
			slog(LG_DEBUG, "chanacs_delete_host(): %s -> %s", ca->mychan->name, ca->host);

			node_del(n, &mychan->chanacs);
			node_free(n);

			BlockHeapFree(chanacs_heap, ca);

			cnt.chanacs--;

			return;
		}
	}
}

chanacs_t *chanacs_find(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!myuser))
		return NULL;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (level != 0x0)
		{
			if ((ca->myuser == myuser) && ((ca->level & level) == level))
				return ca;
		}
		else if (ca->myuser == myuser)
			return ca;
	}

	return NULL;
}

chanacs_t *chanacs_find_host(mychan_t *mychan, char *host, uint32_t level)
{
	node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!host))
		return NULL;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (level != 0x0)
		{
			if ((ca->myuser == NULL) && (!match(ca->host, host)) && ((ca->level & level) == level))
				return ca;
		}
		else if ((ca->myuser == NULL) && (!match(ca->host, host)))
			return ca;
	}

	return NULL;
}

uint32_t chanacs_host_flags(mychan_t *mychan, char *host)
{
	node_t *n;
	chanacs_t *ca;
	uint32_t result = 0;

	if ((!mychan) || (!host))
		return 0;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (ca->myuser == NULL && !match(ca->host, host))
			result |= ca->level;
	}

	return result;
}

chanacs_t *chanacs_find_host_literal(mychan_t *mychan, char *host, uint32_t level)
{
	node_t *n;
	chanacs_t *ca;

	if ((!mychan) || (!host))
		return NULL;

	LIST_FOREACH(n, mychan->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		if (level != 0x0)
		{
			if ((ca->myuser == NULL) && (!strcasecmp(ca->host, host)) && ((ca->level & level) == level))
				return ca;
		}
		else if ((ca->myuser == NULL) && (!strcasecmp(ca->host, host)))
			return ca;
	}

	return NULL;
}

chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, uint32_t level)
{
	char host[BUFSIZE];

	if ((!mychan) || (!u))
		return NULL;

	/* construct buffer for user's host */
	strlcpy(host, u->nick, BUFSIZE);
	strlcat(host, "!", BUFSIZE);
	strlcat(host, u->user, BUFSIZE);
	strlcat(host, "@", BUFSIZE);
	strlcat(host, u->vhost, BUFSIZE);

	return chanacs_find_host(mychan, host, level);
}

uint32_t chanacs_host_flags_by_user(mychan_t *mychan, user_t *u)
{
	char host[BUFSIZE];

	if ((!mychan) || (!u))
		return 0;

	/* construct buffer for user's host */
	strlcpy(host, u->nick, BUFSIZE);
	strlcat(host, "!", BUFSIZE);
	strlcat(host, u->user, BUFSIZE);
	strlcat(host, "@", BUFSIZE);
	strlcat(host, u->vhost, BUFSIZE);

	return chanacs_host_flags(mychan, host);
}

chanacs_t *chanacs_find_by_mask(mychan_t *mychan, char *mask, uint32_t level)
{
	myuser_t *mu = myuser_find(mask);

	if (!mychan || !mask)
		return NULL;

	if (mu)
	{
		chanacs_t *ca = chanacs_find(mychan, mu, level);

		if (ca)
			return ca;
	}

	return chanacs_find_host_literal(mychan, mask, level);
}

boolean_t chanacs_user_has_flag(mychan_t *mychan, user_t *u, uint32_t level)
{
	myuser_t *mu;

	if (!mychan || !u)
		return FALSE;

	mu = u->myuser;
	if (mu != NULL)
	{
		if (chanacs_find(mychan, mu, level))
			return TRUE;
	}

	if (chanacs_find_host_by_user(mychan, u, level))
		return TRUE;

	return FALSE;
}

uint32_t chanacs_user_flags(mychan_t *mychan, user_t *u)
{
	myuser_t *mu;
	chanacs_t *ca;
	uint32_t result = 0;

	if (!mychan || !u)
		return FALSE;

	mu = u->myuser;
	if (mu != NULL)
	{
		ca = chanacs_find(mychan, mu, 0);
		if (ca != NULL)
			result |= ca->level;
	}

	result |= chanacs_host_flags_by_user(mychan, u);

	return result;
}

boolean_t chanacs_source_has_flag(mychan_t *mychan, sourceinfo_t *si, uint32_t level)
{
	return si->su != NULL ? chanacs_user_has_flag(mychan, si->su, level) :
		chanacs_find(mychan, si->smu, level) != NULL;
}

uint32_t chanacs_source_flags(mychan_t *mychan, sourceinfo_t *si)
{
	chanacs_t *ca;

	if (si->su != NULL)
	{
		return chanacs_user_flags(mychan, si->su);
	}
	else
	{
		ca = chanacs_find(mychan, si->smu, 0);
		return ca != NULL ? ca->level : 0;
	}
}

/* Change channel access
 *
 * Either mu or hostmask must be specified.
 * Add the flags in *addflags and remove the flags in *removeflags, updating
 * these to reflect the actual change. Only allow changes to restrictflags.
 * Returns true if successful, false if an unallowed change was attempted.
 * -- jilles */
boolean_t chanacs_change(mychan_t *mychan, myuser_t *mu, char *hostmask, uint32_t *addflags, uint32_t *removeflags, uint32_t restrictflags)
{
	chanacs_t *ca;

	if (mychan == NULL)
		return FALSE;
	if (mu == NULL && hostmask == NULL)
	{
		slog(LG_DEBUG, "chanacs_change(): [%s] mu and hostmask both NULL", mychan->name);
		return FALSE;
	}
	if (mu != NULL && hostmask != NULL)
	{
		slog(LG_DEBUG, "chanacs_change(): [%s] mu and hostmask both not NULL", mychan->name);
		return FALSE;
	}
	if (mu != NULL)
	{
		ca = chanacs_find(mychan, mu, 0);
		if (ca == NULL)
		{
			*removeflags = 0;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			chanacs_add(mychan, mu, *addflags);
		}
		else
		{
			*addflags &= ~ca->level;
			*removeflags &= ca->level & ~*addflags;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			/* attempting to remove bad flag? */
			if (~restrictflags & *removeflags)
				return FALSE;
			/* attempting to manipulate user with more privs? */
			if (~restrictflags & ca->level)
				return FALSE;
			ca->level = (ca->level | *addflags) & ~*removeflags;
			if (ca->level == 0)
				chanacs_delete(mychan, mu, ca->level);
		}
	}
	else /* hostmask != NULL */
	{
		ca = chanacs_find_host_literal(mychan, hostmask, 0);
		if (ca == NULL)
		{
			*removeflags = 0;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			chanacs_add_host(mychan, hostmask, *addflags);
		}
		else
		{
			*addflags &= ~ca->level;
			*removeflags &= ca->level & ~*addflags;
			/* no change? */
			if ((*addflags | *removeflags) == 0)
				return TRUE;
			/* attempting to add bad flag? */
			if (~restrictflags & *addflags)
				return FALSE;
			/* attempting to remove bad flag? */
			if (~restrictflags & *removeflags)
				return FALSE;
			/* attempting to manipulate user with more privs? */
			if (~restrictflags & ca->level)
				return FALSE;
			ca->level = (ca->level | *addflags) & ~*removeflags;
			if (ca->level == 0)
				chanacs_delete_host(mychan, hostmask, ca->level);
		}
	}
	return TRUE;
}

/* version that doesn't return the changes made */
boolean_t chanacs_change_simple(mychan_t *mychan, myuser_t *mu, char *hostmask, uint32_t addflags, uint32_t removeflags, uint32_t restrictflags)
{
	uint32_t a, r;

	a = addflags;
	r = removeflags;
	return chanacs_change(mychan, mu, hostmask, &a, &r, restrictflags);
}

/*******************
 * M E T A D A T A *
 *******************/

metadata_t *metadata_add(void *target, int32_t type, const char *name, const char *value)
{
	myuser_t *mu = NULL;
	mychan_t *mc = NULL;
	chanacs_t *ca = NULL;
	metadata_t *md;
	node_t *n;

	if (!name || !value)
		return NULL;

	if (type == METADATA_USER)
		mu = target;
	else if (type == METADATA_CHANNEL)
		mc = target;
	else if (type == METADATA_CHANACS)
		ca = target;
	else
	{
		slog(LG_DEBUG, "metadata_add(): called on unknown type %d", type);
		return NULL;
	}

	if ((md = metadata_find(target, type, name)))
		metadata_delete(target, type, name);

	md = BlockHeapAlloc(metadata_heap);

	md->name = sstrdup(name);
	md->value = sstrdup(value);

	n = node_create();

	if (type == METADATA_USER)
		node_add(md, n, &mu->metadata);
	else if (type == METADATA_CHANNEL)
		node_add(md, n, &mc->metadata);
	else if (type == METADATA_CHANACS)
		node_add(md, n, &ca->metadata);
	else
	{
		slog(LG_DEBUG, "metadata_add(): trying to add metadata to unknown type %d", type);

		free(md->name);
		free(md->value);
		BlockHeapFree(metadata_heap, md);

		return NULL;
	}

	if (!strncmp("private:", md->name, 8))
		md->private = TRUE;

	return md;
}

void metadata_delete(void *target, int32_t type, const char *name)
{
	node_t *n;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	metadata_t *md = metadata_find(target, type, name);

	if (!md)
		return;

	if (type == METADATA_USER)
	{
		mu = target;
		n = node_find(md, &mu->metadata);
		node_del(n, &mu->metadata);
		node_free(n);
	}
	else if (type == METADATA_CHANNEL)
	{
		mc = target;
		n = node_find(md, &mc->metadata);
		node_del(n, &mc->metadata);
		node_free(n);
	}
	else if (type == METADATA_CHANACS)
	{
		ca = target;
		n = node_find(md, &ca->metadata);
		node_del(n, &ca->metadata);
		node_free(n);
	}
	else
	{
		slog(LG_DEBUG, "metadata_delete(): trying to delete metadata from unknown type %d", type);
		return;
	}

	free(md->name);
	free(md->value);

	BlockHeapFree(metadata_heap, md);
}

metadata_t *metadata_find(void *target, int32_t type, const char *name)
{
	node_t *n;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	list_t *l = NULL;
	metadata_t *md;

	if (!name)
		return NULL;

	if (type == METADATA_USER)
	{
		mu = target;
		l = &mu->metadata;
	}
	else if (type == METADATA_CHANNEL)
	{
		mc = target;
		l = &mc->metadata;
	}
	else if (type == METADATA_CHANACS)
	{
		ca = target;
		l = &ca->metadata;
	}
	else
	{
		slog(LG_DEBUG, "metadata_find(): trying to lookup metadata on unknown type %d", type);
		return NULL;
	}

	LIST_FOREACH(n, l->head)
	{
		md = n->data;

		if (!strcasecmp(md->name, name))
			return md;
	}

	return NULL;
}

static int expire_myuser_cb(dictionary_elem_t *delem, void *unused)
{
	myuser_t *mu = (myuser_t *) delem->node.data;

	/* If they're logged in, update lastlogin time.
	 * To decrease db traffic, may want to only do
	 * this if the account would otherwise be
	 * deleted. -- jilles 
	 */
	if (LIST_LENGTH(&mu->logins) > 0)
	{
		mu->lastlogin = CURRTIME;
		return 0;
	}

	if (MU_HOLD & mu->flags)
		return 0;

	if (((CURRTIME - mu->lastlogin) >= config_options.expire) || ((mu->flags & MU_WAITAUTH) && (CURRTIME - mu->registered >= 86400)))
	{
		/* Don't expire accounts with privs on them,
		 * otherwise someone can reregister
		 * them and take the privs -- jilles */
		if (is_soper(mu))
			return 0;

		snoop("EXPIRE: \2%s\2 from \2%s\2 ", mu->name, mu->email);
		myuser_delete(mu);
	}

	return 0;
}

void expire_check(void *arg)
{
	uint32_t i;
	mychan_t *mc;
	dictionary_iteration_state_t state;

	/* Let them know about this and the likely subsequent db_save()
	 * right away -- jilles */
	sendq_flush(curr_uplink->conn);

	if (config_options.expire == 0)
		return;

	dictionary_foreach(mulist, expire_myuser_cb, NULL);

	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		if ((CURRTIME - mc->used) >= 86400 - 3660)
		{
			/* keep last used time accurate to
			 * within a day, making sure an active
			 * channel will never get "Last used"
			 * in /cs info -- jilles */
			if (mychan_isused(mc))
			{
				mc->used = CURRTIME;
				slog(LG_DEBUG, "expire_check(): updating last used time on %s because it appears to be still in use", mc->name);
				continue;
			}
		}

		if ((CURRTIME - mc->used) >= config_options.expire)
		{
			if (MU_HOLD & mc->founder->flags)
				continue;

			if (MC_HOLD & mc->flags)
				continue;

			snoop("EXPIRE: \2%s\2 from \2%s\2", mc->name, mc->founder->name);

			hook_call_event("channel_drop", mc);
			if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
				part(mc->name, chansvs.nick);

			mychan_delete(mc->name);
		}
	}
}

static int check_myuser_cb(dictionary_elem_t *delem, void *unused)
{
	myuser_t *mu = (myuser_t *) delem->node.data;

	if (MU_OLD_ALIAS & mu->flags)
	{
		slog(LG_INFO, "db_check(): converting previously linked nick %s to a standalone nick", mu->name);
		mu->flags &= ~MU_OLD_ALIAS;
		metadata_delete(mu, METADATA_USER, "private:alias:parent");
	}

	return 0;
}

void db_check()
{
	uint32_t i;
	mychan_t *mc;
	dictionary_iteration_state_t state;

	dictionary_foreach(mulist, check_myuser_cb, NULL);

	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		if (!chanacs_find(mc, mc->founder, CA_FLAGS))
		{
			slog(LG_INFO, "db_check(): adding access for founder on channel %s", mc->name);
			chanacs_change_simple(mc, mc->founder, NULL, CA_FOUNDER_0, 0, CA_ALL);
		}
	}
}
