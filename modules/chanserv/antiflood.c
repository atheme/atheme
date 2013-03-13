/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/antiflood", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org/>"
);

static int antiflood_msg_time = 60;
static int antiflood_msg_count = 5;
static int antiflood_lne_time = 3;

static void on_channel_message(hook_cmessage_data_t *data);

typedef enum {
	MQ_ENFORCE_NONE = 0,
	MQ_ENFORCE_MSG,
	MQ_ENFORCE_LINE,
} mqueue_enforce_strategy_t;

typedef struct {
	char *name;
	size_t max;
	time_t last_used;
	mowgli_list_t entries;
} mqueue_t;

typedef struct {
	time_t time;
	char *message;
	mowgli_node_t node;
} msg_t;

static mowgli_heap_t *msg_heap = NULL;

static void
msg_destroy(msg_t *msg, mqueue_t *mq)
{
	free(msg->message);
	mowgli_node_delete(&msg->node, &mq->entries);

	mowgli_heap_free(msg_heap, msg);
}

static msg_t *
msg_create(mqueue_t *mq, const char *message)
{
	msg_t *msg;

	msg = mowgli_heap_alloc(msg_heap);
	msg->message = sstrdup(message);
	msg->time = CURRTIME;

	if (MOWGLI_LIST_LENGTH(&mq->entries) > mq->max)
	{
		msg_t *head_msg = mq->entries.head->data;
		msg_destroy(head_msg, mq);
	}

	mowgli_node_add(msg, &msg->node, &mq->entries);
	mq->last_used = CURRTIME;

	return msg;
}

static mowgli_patricia_t *mqueue_trie = NULL;
static mowgli_heap_t *mqueue_heap = NULL;
static mowgli_eventloop_timer_t *mqueue_gc_timer = NULL;

static mqueue_t *
mqueue_create(const char *name)
{
	mqueue_t *mq;

	mq = mowgli_heap_alloc(mqueue_heap);
	mq->name = sstrdup(name);
	mq->last_used = CURRTIME;

	mowgli_patricia_add(mqueue_trie, mq->name, mq);

	return mq;
}

static void
mqueue_free(mqueue_t *mq)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, mq->entries.head)
	{
		msg_t *msg = n->data;

		msg_destroy(msg, mq);
	}

	free(mq->name);
	mowgli_heap_free(mqueue_heap, mq);
}

static mqueue_t *
mqueue_get(mychan_t *mc)
{
	mqueue_t *mq;

	mq = mowgli_patricia_retrieve(mqueue_trie, mc->name);
	if (mq == NULL)
		mq = mqueue_create(mc->name);

	return mq;
}

static void
mqueue_destroy(mqueue_t *mq)
{
	mowgli_patricia_delete(mqueue_trie, mq->name);

	mqueue_free(mq);
}

static void
mqueue_trie_destroy_cb(const char *key, void *data, void *privdata)
{
	mqueue_t *mq = data;

	mqueue_free(mq);
}

static void
mqueue_gc(void *unused)
{
	mowgli_patricia_iteration_state_t iter;
	mqueue_t *mq;

	MOWGLI_PATRICIA_FOREACH(mq, &iter, mqueue_trie)
	{
		if ((mq->last_used + 3600) < CURRTIME)
			mqueue_destroy(mq);
	}
}

static mqueue_enforce_strategy_t
mqueue_should_enforce(mqueue_t *mq)
{
	msg_t *oldest, *newest;
	time_t age_delta;

	if (MOWGLI_LIST_LENGTH(&mq->entries) < mq->max)
		return MQ_ENFORCE_NONE;

	oldest = mq->entries.head->data;
	newest = mq->entries.tail->data;

	if (oldest == NULL || newest == NULL || oldest == newest)
		return MQ_ENFORCE_NONE;

	age_delta = newest->time - oldest->time;

	if (age_delta <= antiflood_lne_time)
		return MQ_ENFORCE_LINE;

	if (age_delta <= antiflood_msg_time)
	{
		mowgli_node_t *n;
		bool enforce = true;

		MOWGLI_ITER_FOREACH(n, mq->entries.head)
		{
			msg_t *msg = n->data;

			if (strcasecmp(msg->message, newest->message))
				enforce = false;
		}

		if (enforce)
			return MQ_ENFORCE_MSG;
	}

	return MQ_ENFORCE_NONE;
}

static void
on_channel_message(hook_cmessage_data_t *data)
{
	mychan_t *mc;
	mqueue_t *mq;
	msg_t *msg;

	return_if_fail(data != NULL);
	return_if_fail(data->msg != NULL);
	return_if_fail(data->u != NULL);
	return_if_fail(data->c != NULL);

	mc = MYCHAN_FROM(data->c);
	if (mc == NULL)
		return;

	mq = mqueue_get(mc);
	return_if_fail(mq != NULL);

	msg = msg_create(mq, data->msg);
}

void
_modinit(module_t *m)
{
	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);

	msg_heap = sharedheap_get(sizeof(msg_t));

	mqueue_heap = sharedheap_get(sizeof(mqueue_t));
	mqueue_trie = mowgli_patricia_create(irccasecanon);
	mqueue_gc_timer = mowgli_timer_add(base_eventloop, "mqueue_gc", mqueue_gc, NULL, 300);
}

void
_moddeinit(module_unload_intent_t intent)
{
	hook_del_channel_message(on_channel_message);

	mowgli_patricia_destroy(mqueue_trie, mqueue_trie_destroy_cb, NULL);
	mowgli_timer_destroy(base_eventloop, mqueue_gc_timer);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
