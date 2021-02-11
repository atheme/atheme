/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * chanfix - channel fixing service
 */

#include <atheme.h>
#include "chanfix.h"

#define CFDB_VERSION	1

static unsigned int loading_cfdbv = 0;

static mowgli_heap_t *chanfix_channel_heap = NULL;
static mowgli_heap_t *chanfix_oprecord_heap = NULL;
static mowgli_eventloop_timer_t *chanfix_gather_timer = NULL;
static mowgli_eventloop_timer_t *chanfix_expire_timer = NULL;

mowgli_patricia_t *chanfix_channels = NULL;

struct chanfix_oprecord *
chanfix_oprecord_create(struct chanfix_channel *chan, struct user *u)
{
	struct chanfix_oprecord *orec;

	return_val_if_fail(chan != NULL, NULL);

	if (u != NULL)
	{
		return_val_if_fail((orec = chanfix_oprecord_find(chan, u)) == NULL, orec);
	}

	orec = mowgli_heap_alloc(chanfix_oprecord_heap);

	orec->chan = chan;

	orec->firstseen = CURRTIME;
	orec->lastevent = CURRTIME;

	orec->age = 1;

	if (u != NULL)
	{
		orec->entity = entity(u->myuser);

		mowgli_strlcpy(orec->user, u->user, sizeof orec->user);
		mowgli_strlcpy(orec->host, u->vhost, sizeof orec->host);
	}

	mowgli_node_add(orec, &orec->node, &chan->oprecords);

	return orec;
}

struct chanfix_oprecord *
chanfix_oprecord_find(struct chanfix_channel *chan, struct user *u)
{
	mowgli_node_t *n;

	return_val_if_fail(chan != NULL, NULL);
	return_val_if_fail(u != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
	{
		struct chanfix_oprecord *orec = n->data;

		if (orec->entity != NULL && orec->entity == entity(u->myuser))
			return orec;

		if (!irccasecmp(orec->user, u->user) && !irccasecmp(orec->host, u->vhost))
			return orec;
	}

	return NULL;
}

void
chanfix_oprecord_update(struct chanfix_channel *chan, struct user *u)
{
	struct chanfix_oprecord *orec;

	return_if_fail(chan != NULL);
	return_if_fail(u != NULL);

	orec = chanfix_oprecord_find(chan, u);
	if (orec != NULL)
	{
		orec->age++;
		orec->lastevent = CURRTIME;

		if (orec->entity == NULL && u->myuser != NULL)
			orec->entity = entity(u->myuser);

		return;
	}

	chanfix_oprecord_create(chan, u);
	chan->lastupdate = CURRTIME;
}

void
chanfix_oprecord_delete(struct chanfix_oprecord *orec)
{
	return_if_fail(orec != NULL);

	mowgli_node_delete(&orec->node, &orec->chan->oprecords);
	mowgli_heap_free(chanfix_oprecord_heap, orec);
}

static void
chanfix_channel_delete(struct chanfix_channel *c)
{
	mowgli_node_t *n, *tn;

	return_if_fail(c != NULL);

	mowgli_patricia_delete(chanfix_channels, c->name);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, c->oprecords.head)
	{
		struct chanfix_oprecord *orec = n->data;

		chanfix_oprecord_delete(orec);
	}

	sfree(c->name);
	mowgli_heap_free(chanfix_channel_heap, c);
}

struct chanfix_channel *
chanfix_channel_create(const char *name, struct channel *chan)
{
	struct chanfix_channel *c;

	return_val_if_fail(name != NULL, NULL);

	c = mowgli_heap_alloc(chanfix_channel_heap);
	atheme_object_init(atheme_object(c), name, (atheme_object_destructor_fn) chanfix_channel_delete);

	c->name = sstrdup(name);
	c->chan = chan;
	c->fix_started = 0;

	if (c->chan != NULL)
		c->ts = c->chan->ts;

	mowgli_patricia_add(chanfix_channels, c->name, c);

	return c;
}

struct chanfix_channel *
chanfix_channel_find(const char *name)
{
	return mowgli_patricia_retrieve(chanfix_channels, name);
}

struct chanfix_channel *
chanfix_channel_get(struct channel *chan)
{
	return_val_if_fail(chan != NULL, NULL);

	return mowgli_patricia_retrieve(chanfix_channels, chan->name);
}

static void
chanfix_channel_add_ev(struct channel *ch)
{
	struct chanfix_channel *chan;

	return_if_fail(ch != NULL);

	if ((chan = chanfix_channel_get(ch)) != NULL)
	{
		chan->chan = ch;
		return;
	}

	chanfix_channel_create(ch->name, ch);
}

static void
chanfix_channel_delete_ev(struct channel *ch)
{
	struct chanfix_channel *chan;

	return_if_fail(ch != NULL);

	if ((chan = chanfix_channel_get(ch)) != NULL)
	{
		chan->chan = NULL;
		return;
	}

	chanfix_channel_create(ch->name, NULL);
}

void
chanfix_gather(void *unused)
{
	struct channel *ch;
	mowgli_patricia_iteration_state_t state;
	unsigned int chans = 0, oprecords = 0;

	MOWGLI_PATRICIA_FOREACH(ch, &state, chanlist)
	{
		struct mychan *mc;
		mowgli_node_t *n;
		struct chanfix_channel *chan;

		if ((mc = mychan_find(ch->name)) != NULL)
			continue;

		chan = chanfix_channel_get(ch);
		if (chan == NULL)
			chan = chanfix_channel_create(ch->name, ch);

		MOWGLI_ITER_FOREACH(n, ch->members.head)
		{
			struct chanuser *cu = n->data;

			if (cu->modes & CSTATUS_OP)
			{
				chanfix_oprecord_update(chan, cu->user);
				oprecords++;
			}
		}

		chans++;
	}

	slog(LG_DEBUG, "chanfix_gather(): gathered %u channels and %u oprecords.", chans, oprecords);
}

void
chanfix_expire(void *unused)
{
	struct chanfix_channel *chan;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(chan, &state, chanfix_channels)
	{
		mowgli_node_t *n, *tn;

		MOWGLI_ITER_FOREACH_SAFE(n, tn, chan->oprecords.head)
		{
			struct chanfix_oprecord *orec = n->data;

			/* Simple exponential decay, rounding the decay up
			 * so that low scores expire sooner.
			 */
			orec->age -= (orec->age + CHANFIX_EXPIRE_DIVISOR - 1) /
				CHANFIX_EXPIRE_DIVISOR;

			if (orec->age > 0 && CURRTIME - orec->lastevent < CHANFIX_RETENTION_TIME)
				continue;

			chanfix_oprecord_delete(orec);
		}

		if (MOWGLI_LIST_LENGTH(&chan->oprecords) > 0 &&
				CURRTIME - chan->lastupdate < CHANFIX_RETENTION_TIME)
			continue;

		atheme_object_unref(chan);
	}
}

static void
write_chanfixdb(struct database_handle *db)
{
	struct chanfix_channel *chan;
	mowgli_patricia_iteration_state_t state;

	return_if_fail(db != NULL);

	db_start_row(db, "CFDBV");
	db_write_uint(db, CFDB_VERSION);
	db_commit_row(db);

	MOWGLI_PATRICIA_FOREACH(chan, &state, chanfix_channels)
	{
		mowgli_node_t *n;

		db_start_row(db, "CFCHAN");
		db_write_word(db, chan->name);
		db_write_time(db, chan->ts);
		db_write_time(db, chan->lastupdate);
		db_commit_row(db);

		MOWGLI_ITER_FOREACH(n, chan->oprecords.head)
		{
			struct chanfix_oprecord *orec = n->data;

			db_start_row(db, "CFOP");
			db_write_word(db, chan->name);

			if (orec->entity)
				db_write_word(db, orec->entity->name);
			else
				db_write_word(db, "*");

			db_write_word(db, orec->user);
			db_write_word(db, orec->host);

			db_write_time(db, orec->firstseen);
			db_write_time(db, orec->lastevent);

			db_write_uint(db, orec->age);

			db_commit_row(db);
		}

		if (atheme_object(chan)->metadata != NULL)
		{
			mowgli_patricia_iteration_state_t state2;
			struct metadata *md;

			MOWGLI_PATRICIA_FOREACH(md, &state2, atheme_object(chan)->metadata)
			{
				db_start_row(db, "CFMD");
				db_write_word(db, chan->name);
				db_write_word(db, md->name);
				db_write_str(db, md->value);
				db_commit_row(db);
			}
		}
	}
}

static void
db_h_cfdbv(struct database_handle *db, const char *type)
{
	loading_cfdbv = db_sread_uint(db);
	slog(LG_INFO, "chanfix: opensex data schema version is %u.", loading_cfdbv);
}

static void
db_h_cfchan(struct database_handle *db, const char *type)
{
	const char *name;
	time_t ts, lastupdate;
	struct chanfix_channel *chan;

	name = db_sread_word(db);
	ts = db_sread_time(db);
	lastupdate = db_sread_time(db);

	chan = chanfix_channel_create(name, NULL);
	chan->ts = ts;
	chan->lastupdate = lastupdate;
}

static void
db_h_cfop(struct database_handle *db, const char *type)
{
	const char *name, *entity, *user, *host;
	time_t firstseen, lastevent;
	unsigned int age;
	struct chanfix_channel *chan;
	struct chanfix_oprecord *orec;

	name = db_sread_word(db);
	entity = db_sread_word(db);
	user = db_sread_word(db);
	host = db_sread_word(db);

	firstseen = db_sread_time(db);
	lastevent = db_sread_time(db);

	age = db_sread_uint(db);

	chan = chanfix_channel_find(name);
	orec = chanfix_oprecord_create(chan, NULL);

	orec->entity = myentity_find(entity);
	mowgli_strlcpy(orec->user, user, sizeof orec->user);
	mowgli_strlcpy(orec->host, host, sizeof orec->host);

	orec->firstseen = firstseen;
	orec->lastevent = lastevent;

	orec->age = age;
}

static void
db_h_cfmd(struct database_handle *db, const char *type)
{
	const char *chname, *key, *value;
	struct chanfix_channel *chan;

	chname = db_sread_word(db);
	key = db_sread_word(db);
	value = db_sread_str(db);

	chan = chanfix_channel_find(chname);
	metadata_add(chan, key, value);
}

void
chanfix_gather_init(struct chanfix_persist_record *rec)
{
	hook_add_db_write(write_chanfixdb);
	hook_add_channel_add(chanfix_channel_add_ev);
	hook_add_channel_delete(chanfix_channel_delete_ev);

	db_register_type_handler("CFDBV", db_h_cfdbv);
	db_register_type_handler("CFCHAN", db_h_cfchan);
	db_register_type_handler("CFOP", db_h_cfop);
	db_register_type_handler("CFMD", db_h_cfmd);

	chanfix_expire_timer = mowgli_timer_add(base_eventloop, "chanfix_expire", chanfix_expire, NULL, CHANFIX_EXPIRE_INTERVAL);
	chanfix_gather_timer = mowgli_timer_add(base_eventloop, "chanfix_gather", chanfix_gather, NULL, CHANFIX_GATHER_INTERVAL);

	if (rec != NULL)
	{
		chanfix_channel_heap = rec->chanfix_channel_heap;
		chanfix_oprecord_heap = rec->chanfix_oprecord_heap;

		chanfix_channels = rec->chanfix_channels;
		return;
	}

	chanfix_channel_heap = mowgli_heap_create(sizeof(struct chanfix_channel), 32, BH_LAZY);
	chanfix_oprecord_heap = mowgli_heap_create(sizeof(struct chanfix_oprecord), 32, BH_LAZY);

	chanfix_channels = mowgli_patricia_create(irccasecanon);
}

void
chanfix_gather_deinit(struct chanfix_persist_record *rec)
{
	hook_del_db_write(write_chanfixdb);
	hook_del_channel_add(chanfix_channel_add_ev);
	hook_del_channel_delete(chanfix_channel_delete_ev);

	db_unregister_type_handler("CFDBV");
	db_unregister_type_handler("CFCHAN");
	db_unregister_type_handler("CFOP");
	db_unregister_type_handler("CFMD");

	mowgli_timer_destroy(base_eventloop, chanfix_expire_timer);
	mowgli_timer_destroy(base_eventloop, chanfix_gather_timer);

	rec->chanfix_channel_heap  = chanfix_channel_heap;
	rec->chanfix_oprecord_heap = chanfix_oprecord_heap;
	rec->chanfix_channels      = chanfix_channels;
}
