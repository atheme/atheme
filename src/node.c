/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains data structures, and functions to
 * manipulate them.
 *
 * $Id: node.c 5640 2006-07-02 00:48:37Z jilles $
 */

#include "atheme.h"

list_t operclasslist;
list_t soperlist;
list_t svs_ignore_list;
list_t tldlist;
list_t uplinks;
list_t klnlist;
list_t sidlist[HASHSIZE];
list_t servlist[HASHSIZE];
list_t userlist[HASHSIZE];
list_t uidlist[HASHSIZE];
list_t chanlist[HASHSIZE];
list_t mclist[HASHSIZE];
list_t mulist[HASHSIZE];

list_t sendq;

static BlockHeap *operclass_heap;
static BlockHeap *soper_heap;
static BlockHeap *svsignore_heap;
static BlockHeap *tld_heap;
static BlockHeap *serv_heap;
static BlockHeap *user_heap;
static BlockHeap *chan_heap;

static BlockHeap *chanuser_heap;
static BlockHeap *chanban_heap;
static BlockHeap *uplink_heap;

static BlockHeap *kline_heap;	/* 16 */
static BlockHeap *myuser_heap;	/* HEAP_USER */
static BlockHeap *mychan_heap;	/* HEAP_CHANNEL */
static BlockHeap *chanacs_heap;	/* HEAP_CHANACS */
static BlockHeap *metadata_heap;	/* HEAP_CHANUSER */

static boolean_t mychan_isused(mychan_t *mc);
static myuser_t *mychan_pick_candidate(mychan_t *mc, uint32_t minlevel, int maxtime);
static myuser_t *mychan_pick_successor(mychan_t *mc);

/*************
 * L I S T S *
 *************/

void init_nodes(void)
{
	operclass_heap = BlockHeapCreate(sizeof(operclass_t), 2);
	soper_heap = BlockHeapCreate(sizeof(soper_t), 2);
	svsignore_heap = BlockHeapCreate(sizeof(svsignore_t), 2);
	tld_heap = BlockHeapCreate(sizeof(tld_t), 4);
	serv_heap = BlockHeapCreate(sizeof(server_t), HEAP_SERVER);
	user_heap = BlockHeapCreate(sizeof(user_t), HEAP_USER);
	chan_heap = BlockHeapCreate(sizeof(channel_t), HEAP_CHANNEL);
	chanuser_heap = BlockHeapCreate(sizeof(chanuser_t), HEAP_CHANUSER);
	chanban_heap = BlockHeapCreate(sizeof(chanban_t), HEAP_CHANUSER);
	uplink_heap = BlockHeapCreate(sizeof(uplink_t), 4);
	metadata_heap = BlockHeapCreate(sizeof(metadata_t), HEAP_CHANUSER);
	kline_heap = BlockHeapCreate(sizeof(kline_t), 16);
	myuser_heap = BlockHeapCreate(sizeof(myuser_t), HEAP_USER);
	mychan_heap = BlockHeapCreate(sizeof(mychan_t), HEAP_CHANNEL);
	chanacs_heap = BlockHeapCreate(sizeof(chanacs_t), HEAP_CHANUSER);

	if (!tld_heap || !serv_heap || !user_heap || !chan_heap || !soper_heap || !chanuser_heap || !chanban_heap || !uplink_heap || !metadata_heap || !kline_heap || !myuser_heap || !mychan_heap
	    || !chanacs_heap || !svsignore_heap)
	{
		slog(LG_INFO, "init_nodes(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
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

/*********************
 * S V S I G N O R E *
 *********************/

svsignore_t *svsignore_add(char *mask, char *reason)
{
	svsignore_t *svsignore;
	node_t *n = node_create();

	svsignore = BlockHeapAlloc(svsignore_heap);

	node_add(svsignore, n, &svs_ignore_list);

	svsignore->mask = sstrdup(mask);
	svsignore->settime = CURRTIME;
	svsignore->reason = sstrdup(reason);
	cnt.svsignore++;

	return svsignore;
}

svsignore_t *svsignore_find(user_t *source)
{
	svsignore_t *svsignore;
	node_t *n;
	char host[BUFSIZE];

	*host = '\0';
	strlcpy(host, source->nick, BUFSIZE);
	strlcat(host, "!", BUFSIZE);
	strlcat(host, source->user, BUFSIZE);
	strlcat(host, "@", BUFSIZE);
	strlcat(host, source->host, BUFSIZE);

	LIST_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		if (!match(svsignore->mask, host))
			return svsignore;
	}

	return NULL;

}


/***********
 * T L D S *
 ***********/

tld_t *tld_add(char *name)
{
	tld_t *tld;
	node_t *n = node_create();

	slog(LG_DEBUG, "tld_add(): %s", name);

	tld = BlockHeapAlloc(tld_heap);

	node_add(tld, n, &tldlist);

	tld->name = sstrdup(name);

	cnt.tld++;

	return tld;
}

void tld_delete(char *name)
{
	tld_t *tld = tld_find(name);
	node_t *n;

	if (!tld)
	{
		slog(LG_DEBUG, "tld_delete(): called for nonexistant tld: %s", name);

		return;
	}

	slog(LG_DEBUG, "tld_delete(): %s", tld->name);

	n = node_find(tld, &tldlist);
	node_del(n, &tldlist);
	node_free(n);

	free(tld->name);
	BlockHeapFree(tld_heap, tld);

	cnt.tld--;
}

tld_t *tld_find(char *name)
{
	tld_t *tld;
	node_t *n;

	LIST_FOREACH(n, tldlist.head)
	{
		tld = (tld_t *)n->data;

		if (!strcasecmp(name, tld->name))
			return tld;
	}

	return NULL;
}

/*****************
 * S E R V E R S *
 *****************/

server_t *server_add(char *name, uint8_t hops, char *uplink, char *id, char *desc)
{
	server_t *s, *u = NULL;
	node_t *n = node_create();
	char *tld;

	if (uplink)
	{
		slog(LG_DEBUG, "server_add(): %s, uplink %s", name, uplink);
		u = server_find(uplink);
	}
	else
		slog(LG_DEBUG, "server_add(): %s, root", name);

	s = BlockHeapAlloc(serv_heap);

	s->hash = SHASH((unsigned char *)name);

	node_add(s, n, &servlist[s->hash]);

	if (id != NULL)
	{
		s->sid = sstrdup(id);
		s->shash = SHASH((unsigned char *)id);
		node_add(s, node_create(), &sidlist[s->shash]);
	}

	/* check to see if it's hidden */
	if (!strncmp(desc, "(H)", 3))
	{
		s->flags |= SF_HIDE;
		desc += 3;
		if (*desc == ' ')
			desc++;
	}

	s->name = sstrdup(name);
	s->desc = sstrdup(desc);
	s->hops = hops;
	s->connected_since = CURRTIME;

	if (u)
	{
		s->uplink = u;
		node_add(s, node_create(), &u->children);
	}

	/* tld list for global noticer */
	tld = strrchr(name, '.');

	if (!tld_find(tld))
		tld_add(tld);

	cnt.server++;

	return s;
}

void server_delete(char *name)
{
	server_t *s = server_find(name);
	server_t *child;
	user_t *u;
	node_t *n, *tn;

	if (!s)
	{
		slog(LG_DEBUG, "server_delete(): called for nonexistant server: %s", name);

		return;
	}
	if (s == me.me)
	{
		/* Deleting this would cause confusion, so let's not do it.
		 * Some ircds send SQUIT <myname> when atheme is squitted.
		 * -- jilles */
		slog(LG_DEBUG, "server_delete(): tried to delete myself");
		return;
	}

	slog(LG_DEBUG, "server_delete(): %s", s->name);

	/* first go through it's users and kill all of them */
	LIST_FOREACH_SAFE(n, tn, s->userlist.head)
	{
		u = (user_t *)n->data;
		user_delete(u);
	}

	LIST_FOREACH_SAFE(n, tn, s->children.head)
	{
		child = n->data;
		server_delete(child->name);
	}

	/* now remove the server */
	n = node_find(s, &servlist[s->hash]);
	node_del(n, &servlist[s->hash]);
	node_free(n);

	if (s->sid)
	{
		n = node_find(s, &sidlist[s->shash]);
		node_del(n, &sidlist[s->shash]);
		node_free(n);
	}

	if (s->uplink)
	{
		n = node_find(s, &s->uplink->children);
		node_del(n, &s->uplink->children);
		node_free(n);
	}

	free(s->name);
	free(s->desc);
	if (s->sid)
		free(s->sid);

	BlockHeapFree(serv_heap, s);

	cnt.server--;
}

server_t *server_find(char *name)
{
	server_t *s;
	node_t *n;

	if (ircd->uses_uid == TRUE)
	{
		LIST_FOREACH(n, sidlist[SHASH((unsigned char *)name)].head)
		{
			s = (server_t *)n->data;

			if (!strcasecmp(name, s->sid))
				return s;
		}
	}

	LIST_FOREACH(n, servlist[SHASH((unsigned char *)name)].head)
	{
		s = (server_t *)n->data;

		if (!irccasecmp(name, s->name))
			return s;
	}

	return NULL;
}

/*************
 * U S E R S *
 *************/

user_t *user_add(char *nick, char *user, char *host, char *vhost, char *ip, char *uid, char *gecos, server_t *server, uint32_t ts)
{
	user_t *u;
	node_t *n = node_create();

	slog(LG_DEBUG, "user_add(): %s (%s@%s) -> %s", nick, user, host, server->name);

	u = BlockHeapAlloc(user_heap);

	u->hash = UHASH((unsigned char *)nick);

	if (uid != NULL)
	{
		strlcpy(u->uid, uid, NICKLEN);
		u->uhash = UHASH((unsigned char *)uid);
		node_add(u, node_create(), &uidlist[u->uhash]);
	}

	node_add(u, n, &userlist[u->hash]);

	strlcpy(u->nick, nick, NICKLEN);
	strlcpy(u->user, user, USERLEN);
	strlcpy(u->host, host, HOSTLEN);
	strlcpy(u->gecos, gecos, GECOSLEN);

	if (vhost)
		strlcpy(u->vhost, vhost, HOSTLEN);
	else
		strlcpy(u->vhost, host, HOSTLEN);

	if (ip && strcmp(ip, "0") && strcmp(ip, "0.0.0.0") && strcmp(ip, "255.255.255.255"))
		strlcpy(u->ip, ip, HOSTLEN);

	u->server = server;
	u->server->users++;
	node_add(u, node_create(), &u->server->userlist);

	if (ts)
		u->ts = ts;
	else
		u->ts = CURRTIME;

	cnt.user++;

	hook_call_event("user_add", u);

	return u;
}

void user_delete(user_t *u)
{
	node_t *n, *tn;
	chanuser_t *cu;

	if (!u)
	{
		slog(LG_DEBUG, "user_delete(): called for NULL user");
		return;
	}

	slog(LG_DEBUG, "user_delete(): removing user: %s -> %s", u->nick, u->server->name);

	hook_call_event("user_delete", u);

	u->server->users--;
	if (is_ircop(u))
		u->server->opers--;
	if (u->flags & UF_INVIS)
		u->server->invis--;

	/* remove the user from each channel */
	LIST_FOREACH_SAFE(n, tn, u->channels.head)
	{
		cu = (chanuser_t *)n->data;

		chanuser_delete(cu->chan, u);
	}

	n = node_find(u, &userlist[u->hash]);
	node_del(n, &userlist[u->hash]);
	node_free(n);

	if (*u->uid)
	{
		n = node_find(u, &uidlist[u->uhash]);
		node_del(n, &uidlist[u->uhash]);
		node_free(n);
	}

	n = node_find(u, &u->server->userlist);
	node_del(n, &u->server->userlist);
	node_free(n);

	if (u->myuser)
	{
		LIST_FOREACH_SAFE(n, tn, u->myuser->logins.head)
		{
			if (n->data == u)
			{
				node_del(n, &u->myuser->logins);
				node_free(n);
				break;
			}
		}
		u->myuser = NULL;
	}

	BlockHeapFree(user_heap, u);

	cnt.user--;
}

user_t *user_find(const char *nick)
{
	user_t *u;
	node_t *n;

	if (nick == NULL)
		return NULL;

	if (ircd->uses_uid == TRUE)
	{
		LIST_FOREACH(n, uidlist[SHASH((const unsigned char *)nick)].head)
		{
			u = (user_t *)n->data;

			if (!strcasecmp(nick, u->uid))
				return u;
		}
	}

	LIST_FOREACH(n, userlist[SHASH((const unsigned char *)nick)].head)
	{
		u = (user_t *)n->data;

		if (!irccasecmp(nick, u->nick))
		{
			if (ircd->uses_p10)
				wallops("user_find() found user %s by nick!",
						nick);
			return u;
		}
	}

	return NULL;
}

/* Use this for user input, to prevent users chasing users by UID -- jilles */
user_t *user_find_named(const char *nick)
{
	user_t *u;
	node_t *n;

	LIST_FOREACH(n, userlist[SHASH((const unsigned char *)nick)].head)
	{
		u = (user_t *)n->data;

		if (!irccasecmp(nick, u->nick))
			return u;
	}

	return NULL;
}

/* Change a UID, for services */
void user_changeuid(user_t *u, char *uid)
{
	node_t *n;

	if (*u->uid)
	{
		n = node_find(u, &uidlist[u->uhash]);
		node_del(n, &uidlist[u->uhash]);
		node_free(n);
	}

	strlcpy(u->uid, uid ? uid : "", NICKLEN);

	if (*u->uid)
	{
		u->uhash = UHASH((unsigned char *)uid);
		node_add(u, node_create(), &uidlist[u->uhash]);
	}
}

/*******************
 * C H A N N E L S *
 *******************/
channel_t *channel_add(char *name, uint32_t ts)
{
	channel_t *c;
	mychan_t *mc;
	node_t *n;

	if (*name != '#')
	{
		slog(LG_DEBUG, "channel_add(): got non #channel: %s", name);
		return NULL;
	}

	c = channel_find(name);

	if (c)
	{
		slog(LG_DEBUG, "channel_add(): channel already exists: %s", name);
		return c;
	}

	slog(LG_DEBUG, "channel_add(): %s", name);

	n = node_create();
	c = BlockHeapAlloc(chan_heap);

	c->name = sstrdup(name);
	c->ts = ts;
	c->hash = CHASH((unsigned char *)name);

	c->topic = NULL;
	c->topic_setter = NULL;

	c->bans.head = NULL;
	c->bans.tail = NULL;
	c->bans.count = 0;

	if ((mc = mychan_find(c->name)))
		mc->chan = c;

	node_add(c, n, &chanlist[c->hash]);

	cnt.chan++;

	hook_call_event("channel_add", c);

	if (config_options.chan != NULL && !irccmp(config_options.chan, name))
		joinall(config_options.chan);

	return c;
}

void channel_delete(char *name)
{
	channel_t *c = channel_find(name);
	mychan_t *mc;
	node_t *n, *tn, *n2;
	user_t *u;
	chanuser_t *cu;

	if (!c)
	{
		slog(LG_DEBUG, "channel_delete(): called for nonexistant channel: %s", name);
		return;
	}

	slog(LG_DEBUG, "channel_delete(): %s", c->name);

	/* channels with services may not be empty, kick them out -- jilles */
	LIST_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;
		u = cu->user;
		node_del(n, &c->members);
		node_free(n);
		n2 = node_find(cu, &u->channels);
		node_del(n2, &u->channels);
		node_free(n2);
		BlockHeapFree(chanuser_heap, cu);
		cnt.chanuser--;
	}
	c->nummembers = 0;

	hook_call_event("channel_delete", c);

	/* we assume all lists should be null */

	n = node_find(c, &chanlist[c->hash]);
	node_del(n, &chanlist[c->hash]);
	node_free(n);

	if ((mc = mychan_find(c->name)))
		mc->chan = NULL;

	clear_simple_modes(c);

	free(c->name);
	BlockHeapFree(chan_heap, c);

	cnt.chan--;
}

channel_t *channel_find(const char *name)
{
	channel_t *c;
	node_t *n;

	LIST_FOREACH(n, chanlist[shash((const unsigned char *) name)].head)
	{
		c = (channel_t *)n->data;

		if (!irccasecmp(name, c->name))
			return c;
	}

	return NULL;
}

/********************
 * C H A N  B A N S *
 ********************/

chanban_t *chanban_add(channel_t *chan, const char *mask, int type)
{
	chanban_t *c;
	node_t *n;

	c = chanban_find(chan, mask, type);

	if (c)
	{
		slog(LG_DEBUG, "chanban_add(): channel ban %s:%s already exists", chan->name, c->mask);
		return NULL;
	}

	slog(LG_DEBUG, "chanban_add(): %s +%c %s", chan->name, type, mask);

	n = node_create();
	c = BlockHeapAlloc(chanban_heap);

	c->chan = chan;
	c->mask = sstrdup(mask);
	c->type = type;

	node_add(c, n, &chan->bans);

	return c;
}

void chanban_delete(chanban_t * c)
{
	node_t *n;

	if (!c)
	{
		slog(LG_DEBUG, "chanban_delete(): called for nonexistant ban");
		return;
	}

	n = node_find(c, &c->chan->bans);
	node_del(n, &c->chan->bans);
	node_free(n);

	free(c->mask);
	BlockHeapFree(chanban_heap, c);
}

chanban_t *chanban_find(channel_t *chan, const char *mask, int type)
{
	chanban_t *c;
	node_t *n;

	LIST_FOREACH(n, chan->bans.head)
	{
		c = n->data;

		if (c->type == type && !irccasecmp(c->mask, mask))
			return c;
	}

	return NULL;
}

void chanban_clear(channel_t *chan)
{
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, chan->bans.head)
	{
		/* inefficient but avoids code duplication -- jilles */
		chanban_delete(n->data);
	}
}

/**********************
 * C H A N  U S E R S *
 **********************/

/*
 * Rewritten 06/23/05 by nenolod:
 *
 * Iterate through the list of prefix characters we know about.
 * Continue to do so until all prefixes are covered. Then add the
 * nick to the channel, with the privs he has acquired thus far.
 *
 * Once, and only once we have done that do we start in on checking
 * privileges. Otherwise we have a very inefficient way of doing
 * things. It worked fine for shrike, but the old code was restricted
 * to handling only @, @+ and + as prefixes.
 */
chanuser_t *chanuser_add(channel_t *chan, char *nick)
{
	user_t *u;
	node_t *n1;
	node_t *n2;
	chanuser_t *cu, *tcu;
	uint32_t flags = 0;
	int i = 0;

	if (chan == NULL)
		return NULL;

	if (*chan->name != '#')
	{
		slog(LG_DEBUG, "chanuser_add(): got non #channel: %s", chan->name);
		return NULL;
	}

	while (*nick != '\0')
	{
		for (i = 0; prefix_mode_list[i].mode; i++)
			if (*nick == prefix_mode_list[i].mode)
			{
				flags |= prefix_mode_list[i].value;
				break;
			}
		if (!prefix_mode_list[i].mode)
			break;
		nick++;
	}

	u = user_find(nick);
	if (u == NULL)
	{
		slog(LG_DEBUG, "chanuser_add(): nonexist user: %s", nick);
		return NULL;
	}

	tcu = chanuser_find(chan, u);
	if (tcu != NULL)
	{
		slog(LG_DEBUG, "chanuser_add(): user is already present: %s -> %s", chan->name, u->nick);

		/* could be an OPME or other desyncher... */
		tcu->modes |= flags;

		return tcu;
	}

	slog(LG_DEBUG, "chanuser_add(): %s -> %s", chan->name, u->nick);

	n1 = node_create();
	n2 = node_create();

	cu = BlockHeapAlloc(chanuser_heap);

	cu->chan = chan;
	cu->user = u;
	cu->modes |= flags;

	chan->nummembers++;

	node_add(cu, n1, &chan->members);
	node_add(cu, n2, &u->channels);

	cnt.chanuser++;

	hook_call_event("channel_join", cu);

	return cu;
}

void chanuser_delete(channel_t *chan, user_t *user)
{
	chanuser_t *cu;
	node_t *n, *tn, *n2;

	if (!chan)
	{
		slog(LG_DEBUG, "chanuser_delete(): called with NULL chan");
		return;
	}

	if (!user)
	{
		slog(LG_DEBUG, "chanuser_delete(): called with NULL user");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == user)
		{
			/* this is called BEFORE we remove the user */
			hook_call_event("channel_part", cu);

			slog(LG_DEBUG, "chanuser_delete(): %s -> %s (%d)", cu->chan->name, cu->user->nick, cu->chan->nummembers - 1);
			node_del(n, &chan->members);
			node_free(n);

			n2 = node_find(cu, &user->channels);
			node_del(n2, &user->channels);
			node_free(n2);

			BlockHeapFree(chanuser_heap, cu);

			chan->nummembers--;
			cnt.chanuser--;

			if (chan->nummembers == 0 && !(chan->modes & ircd->perm_mode))
			{
				/* empty channels die */
				slog(LG_DEBUG, "chanuser_delete(): `%s' is empty, removing", chan->name);

				channel_delete(chan->name);
			}

			return;
		}
	}
}

chanuser_t *chanuser_find(channel_t *chan, user_t *user)
{
	node_t *n;
	chanuser_t *cu;

	if ((!chan) || (!user))
		return NULL;

	LIST_FOREACH(n, chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == user)
			return cu;
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

void kline_delete(char *user, char *host)
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

kline_t *kline_find(char *user, char *host)
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
		if (!match(k->user, u->user) && (!match(k->host, u->host) || !match(k->host, u->ip)))
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
 * M Y U S E R *
 ***************/

myuser_t *myuser_add(char *name, char *pass, char *email, uint32_t flags)
{
	myuser_t *mu;
	node_t *n;
	soper_t *soper;

	mu = myuser_find(name);

	if (mu)
	{
		slog(LG_DEBUG, "myuser_add(): myuser already exists: %s", name);
		return mu;
	}

	slog(LG_DEBUG, "myuser_add(): %s -> %s", name, email);

	n = node_create();
	mu = BlockHeapAlloc(myuser_heap);

	/* set the password later */
	strlcpy(mu->name, name, NICKLEN);
	strlcpy(mu->email, email, EMAILLEN);
	mu->registered = CURRTIME;
	mu->hash = MUHASH((unsigned char *)name);
	mu->flags = flags;

	/* If it's already crypted, don't touch the password. Otherwise,
	 * use set_password() to initialize it. Why? Because set_password
	 * will move the user to encrypted passwords if possible. That way,
	 * new registers are immediately protected and the database is
	 * immediately converted the first time we start up with crypto.
	 */
	if (flags & MU_CRYPTPASS)
		strlcpy(mu->pass, pass, NICKLEN);
	else
		set_password(mu, pass);

	node_add(mu, n, &mulist[mu->hash]);

	if ((soper = soper_find_named(mu->name)) != NULL)
	{
		slog(LG_DEBUG, "myuser_add(): user `%s' has been declared as soper, activating privileges.",
			mu->name);
		soper->myuser = mu;
		mu->soper = soper;
	}

	cnt.myuser++;

	return mu;
}

void myuser_delete(myuser_t *mu)
{
	myuser_t *successor;
	mychan_t *mc;
	chanacs_t *ca;
	user_t *u;
	node_t *n, *tn;
	metadata_t *md;
	uint32_t i;

	if (!mu)
	{
		slog(LG_DEBUG, "myuser_delete(): called for NULL myuser");
		return;
	}

	slog(LG_DEBUG, "myuser_delete(): %s", mu->name);

	/* log them out */
	if (authservice_loaded)
	{
		LIST_FOREACH_SAFE(n, tn, mu->logins.head)
		{
			u = (user_t *)n->data;
			if (!ircd_on_logout(u->nick, mu->name, NULL))
			{
				u->myuser = NULL;
				node_del(n, &mu->logins);
				node_free(n);
			}
		}
	}
	
	/* kill all their channels
	 *
	 * We CANNOT do this based soley on chanacs!
	 * A founder could remove all of his flags.
	 */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			/* attempt succession */
			if (mc->founder == mu && (successor = mychan_pick_successor(mc)) != NULL)
			{
				snoop("SUCCESSION: \2%s\2 -> \2%s\2 from \2%s\2", successor->name, mc->name, mc->founder->name);

				chanacs_change_simple(mc, successor, NULL, CA_FOUNDER_0, 0, CA_ALL);
				mc->founder = successor;
	
				myuser_notice(chansvs.nick, mc->founder, "You are now founder on \2%s\2 (as \2%s\2).", mc->name, mc->founder->name);
			}
		
			/* no successor found */
			if (mc->founder == mu)
			{
				snoop("DELETE: \2%s\2 from \2%s\2", mc->name, mu->name);

				hook_call_event("channel_drop", mc);
				if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
					part(mc->name, chansvs.nick);
				mychan_delete(mc->name);
			}
		}
	}
	
	/* remove their chanacs shiz */
	LIST_FOREACH_SAFE(n, tn, mu->chanacs.head)
	{
		ca = (chanacs_t *)n->data;

		chanacs_delete(ca->mychan, ca->myuser, ca->level);
	}

	/* remove them from the soper list */
	if (soper_find(mu))
		soper_delete(mu->soper);

	/* delete the metadata */
	LIST_FOREACH_SAFE(n, tn, mu->metadata.head)
	{
		md = (metadata_t *)n->data;
		metadata_delete(mu, METADATA_USER, md->name);
	}

	/* kill any authcookies */
	authcookie_destroy_all(mu);

	n = node_find(mu, &mulist[mu->hash]);
	node_del(n, &mulist[mu->hash]);
	node_free(n);

	BlockHeapFree(myuser_heap, mu);

	cnt.myuser--;
}

myuser_t *myuser_find(char *name)
{
	myuser_t *mu;
	node_t *n;

	if (name == NULL)
		return NULL;

	LIST_FOREACH(n, mulist[shash((unsigned char *) name)].head)
	{
		mu = (myuser_t *)n->data;

		if (!irccasecmp(name, mu->name))
			return mu;
	}

	return NULL;
}

myuser_t *myuser_find_ext(char *name)
{
	user_t *u;

	if (name == NULL)
		return NULL;

	if (*name == '=')
	{
		u = user_find(name + 1);
		return u != NULL ? u->myuser : NULL;
	}
	else
		return myuser_find(name);
}

void
myuser_notice(char *from, myuser_t *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	node_t *n;
	user_t *u;

	if (target == NULL)
		return;

	/* have to reformat it here, can't give a va_list to notice() :(
	 * -- jilles */
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	LIST_FOREACH(n, target->logins.head)
	{
		u = (user_t *)n->data;
		notice(from, u->nick, "%s", buf);
	}
}

/***************
 * M Y C H A N *
 ***************/

mychan_t *mychan_add(char *name)
{
	mychan_t *mc;
	node_t *n;

	mc = mychan_find(name);

	if (mc)
	{
		slog(LG_DEBUG, "mychan_add(): mychan already exists: %s", name);
		return mc;
	}

	slog(LG_DEBUG, "mychan_add(): %s", name);

	n = node_create();
	mc = BlockHeapAlloc(mychan_heap);

	strlcpy(mc->name, name, CHANNELLEN);
	mc->founder = NULL;
	mc->registered = CURRTIME;
	mc->chan = channel_find(name);
	mc->hash = MCHASH((unsigned char *)name);

	node_add(mc, n, &mclist[mc->hash]);

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

	n = node_find(mc, &mclist[mc->hash]);
	node_del(n, &mclist[mc->hash]);
	node_free(n);

	BlockHeapFree(mychan_heap, mc);

	cnt.mychan--;
}

mychan_t *mychan_find(const char *name)
{
	mychan_t *mc;
	node_t *n;

	LIST_FOREACH(n, mclist[shash((const unsigned char *) name)].head)
	{
		mc = (mychan_t *)n->data;

		if (!irccasecmp(name, mc->name))
			return mc;
	}

	return NULL;
}

/* Check if there is anyone on the channel fulfilling the conditions.
 * Fairly expensive, but this is sometimes necessary to avoid
 * inappropriate drops. -- jilles */
static boolean_t mychan_isused(mychan_t *mc)
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
static myuser_t *mychan_pick_candidate(mychan_t *mc, uint32_t minlevel, int maxtime)
{
	int j, tcnt;
	node_t *n, *n2;
	chanacs_t *ca;
	mychan_t *tmc;
	myuser_t *mu;

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
			for (j = 0; j < HASHSIZE; j++)
			{
				LIST_FOREACH(n2, mclist[j].head)
				{
					tmc = (mychan_t *)n2->data;

					if (is_founder(tmc, mu))
						tcnt++;
				}
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
static myuser_t *mychan_pick_successor(mychan_t *mc)
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

metadata_t *metadata_add(void *target, int32_t type, char *name, char *value)
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

void metadata_delete(void *target, int32_t type, char *name)
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

metadata_t *metadata_find(void *target, int32_t type, char *name)
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

void expire_check(void *arg)
{
	uint32_t i;
	myuser_t *mu;
	mychan_t *mc;
	node_t *n, *tn;

	/* Let them know about this and the likely subsequent db_save()
	 * right away -- jilles */
	sendq_flush(curr_uplink->conn);

	if (config_options.expire == 0)
		return;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			/* If they're logged in, update lastlogin time.
			 * To decrease db traffic, may want to only do
			 * this if the account would otherwise be
			 * deleted. -- jilles 
			 */
			if (LIST_LENGTH(&mu->logins) > 0)
			{
				mu->lastlogin = CURRTIME;
				continue;
			}

			if (MU_HOLD & mu->flags)
				continue;

			if (((CURRTIME - mu->lastlogin) >= config_options.expire) || ((mu->flags & MU_WAITAUTH) && (CURRTIME - mu->registered >= 86400)))
			{
				/* Don't expire accounts with privs on them,
				 * otherwise someone can reregister
				 * them and take the privs -- jilles */
				if (is_soper(mu))
					continue;

				snoop("EXPIRE: \2%s\2 from \2%s\2 ", mu->name, mu->email);
				myuser_delete(mu);
			}
		}
	}

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

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
}

void db_check()
{
	uint32_t i;
	myuser_t *mu;
	mychan_t *mc;
	node_t *n;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			if (MU_OLD_ALIAS & mu->flags)
			{
				slog(LG_INFO, "db_check(): converting previously linked nick %s to a standalone nick", mu->name);
				mu->flags &= ~MU_OLD_ALIAS;
				metadata_delete(mu, METADATA_USER, "private:alias:parent");
			}
		}
	}

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			if (!chanacs_find(mc, mc->founder, CA_FLAGS))
			{
				slog(LG_INFO, "db_check(): adding access for founder on channel %s", mc->name);
				chanacs_change_simple(mc, mc->founder, NULL, CA_FOUNDER_0, 0, CA_ALL);
			}
		}
	}
}
