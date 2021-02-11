/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * chanfix - channel fixing service
 */

#ifndef ATHEME_MOD_CHANFIX_CHANFIX_H
#define ATHEME_MOD_CHANFIX_CHANFIX_H 1

#include <atheme.h>

#define CHANFIX_OP_THRESHHOLD   3U
#define CHANFIX_ACCOUNT_WEIGHT  1.5f
#define CHANFIX_MIN_FIX_SCORE   12U

#define CHANFIX_INITIAL_STEP    0.70f
#define CHANFIX_FINAL_STEP      0.30f

#define CHANFIX_RETENTION_TIME  (4U * SECONDS_PER_WEEK)
#define CHANFIX_FIX_TIME        SECONDS_PER_HOUR
#define CHANFIX_GATHER_INTERVAL (5U * SECONDS_PER_MINUTE)
#define CHANFIX_EXPIRE_INTERVAL SECONDS_PER_HOUR

/* This value has been chosen such that the maximum score is about 8064,
 * which is the number of CHANFIX_GATHER_INTERVALs in CHANFIX_RETENTION_TIME.
 * Higher scores would decay more than they can gain (12 per hour).
 */
#define CHANFIX_EXPIRE_DIVISOR  672U

struct chanfix_channel
{
	struct atheme_object parent;

	char *name;

	mowgli_list_t oprecords;
	time_t ts;
	time_t lastupdate;

	struct channel *chan;

	time_t fix_started;
	bool fix_requested;
};

struct chanfix_oprecord
{
	mowgli_node_t node;

	struct chanfix_channel *chan;

	struct myentity *entity;

	char user[USERLEN + 1];
	char host[HOSTLEN + 1];

	time_t firstseen;
	time_t lastevent;
	unsigned int age;
};

struct chanfix_persist_record
{
	int version;

	mowgli_heap_t *chanfix_channel_heap;
	mowgli_heap_t *chanfix_oprecord_heap;

	mowgli_patricia_t *chanfix_channels;
};

extern struct service *chanfix;
extern mowgli_patricia_t *chanfix_channels;

void chanfix_gather_init(struct chanfix_persist_record *);
void chanfix_gather_deinit(struct chanfix_persist_record *);

void chanfix_oprecord_update(struct chanfix_channel *chan, struct user *u);
void chanfix_oprecord_delete(struct chanfix_oprecord *orec);
struct chanfix_oprecord *chanfix_oprecord_create(struct chanfix_channel *chan, struct user *u);
struct chanfix_oprecord *chanfix_oprecord_find(struct chanfix_channel *chan, struct user *u);
struct chanfix_channel *chanfix_channel_create(const char *name, struct channel *chan);
struct chanfix_channel *chanfix_channel_find(const char *name);
struct chanfix_channel *chanfix_channel_get(struct channel *chan);
void chanfix_gather(void *unused);
void chanfix_expire(void *unused);

extern bool chanfix_do_autofix;
void chanfix_autofix_ev(void *unused);
void chanfix_can_register(struct hook_channel_register_check *req);

extern struct command cmd_list;
extern struct command cmd_chanfix;
extern struct command cmd_scores;
extern struct command cmd_info;
extern struct command cmd_help;
extern struct command cmd_mark;
extern struct command cmd_nofix;

#endif /* !ATHEME_MOD_CHANFIX_CHANFIX_H */
