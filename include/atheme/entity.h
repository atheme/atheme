/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * entity.h - entity tracking
 */

#ifndef ATHEME_INC_ENTITY_H
#define ATHEME_INC_ENTITY_H 1

#include <atheme/constants.h>
#include <atheme/object.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

enum myentity_type
{
	ENT_ANY = 0,
	ENT_USER,
	ENT_GROUP,
	ENT_EXTTARGET,
};

struct myentity
{
	struct atheme_object         parent;
	enum myentity_type           type;
	stringref                    name;
	char                         id[IDLEN + 1];
	mowgli_list_t                chanacs;
	const struct entity_vtable * vtable;
};

#define entity(x)	((struct myentity *)(x))
#define user(x)		(isuser(x) ? (struct myuser *)(x) : NULL)
#define group(x)	(isgroup(x) ? (struct mygroup *)(x) : NULL)
#define isuser(x)	(x != NULL && entity(x)->type == ENT_USER)
#define isgroup(x)	(x != NULL && entity(x)->type == ENT_GROUP)
#define isdynamic(x)	(x != NULL && (entity(x)->type == ENT_EXTTARGET))

void init_entities(void);
void myentity_set_last_uid(const char *last_uid);
const char *myentity_get_last_uid(void);
const char *myentity_alloc_uid(void);

void myentity_put(struct myentity *me);
void myentity_del(struct myentity *me);
struct myentity *myentity_find(const char *name);
struct myentity *myentity_find_ext(const char *name);
struct myentity *myentity_find_uid(const char *uid);

struct myentity_iteration_state
{
	mowgli_patricia_iteration_state_t       st;
	enum myentity_type                      type;
};

void myentity_foreach(int (*cb)(struct myentity *me, void *privdata), void *privdata);
void myentity_foreach_t(enum myentity_type type, int (*cb)(struct myentity *me, void *privdata), void *privdata);
void myentity_foreach_start(struct myentity_iteration_state *state, enum myentity_type type);
void myentity_foreach_next(struct myentity_iteration_state *state);
struct myentity *myentity_foreach_cur(struct myentity_iteration_state *state);

#define MYENTITY_FOREACH_T(elem, state, type) for (myentity_foreach_start(state, type); (elem = myentity_foreach_cur(state)); myentity_foreach_next(state))
#define MYENTITY_FOREACH(elem, state) MYENTITY_FOREACH_T(elem, state, 0)

void myentity_stats(void (*cb)(const char *line, void *privdata), void *privdata);

/* chanacs */
unsigned int myentity_count_channels_with_flagset(struct myentity *mt, unsigned int flagset);
bool myentity_can_register_channel(struct myentity *mt);
bool myentity_allow_foundership(struct myentity *mt);

#endif /* !ATHEME_INC_ENTITY_H */
