/* entity.h - entity tracking
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef ENTITY_H
#define ENTITY_H

enum myentity_type
{
	ENT_ANY = 0,
	ENT_USER,
	ENT_GROUP,
	ENT_EXTTARGET,
};

struct myentity
{
	struct atheme_object parent;
	enum myentity_type type;

	stringref name;
	char id[IDLEN + 1];

	mowgli_list_t chanacs;
	const struct entity_chanacs_validation_vtable *chanacs_validate;
};

#define entity(x)	((struct myentity *)(x))
#define user(x)		(isuser(x) ? (myuser_t *)(x) : NULL)
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

typedef struct {
	mowgli_patricia_iteration_state_t st;
	enum myentity_type type;
} myentity_iteration_state_t;

extern void myentity_foreach(int (*cb)(struct myentity *me, void *privdata), void *privdata);
extern void myentity_foreach_t(enum myentity_type type, int (*cb)(struct myentity *me, void *privdata), void *privdata);
extern void myentity_foreach_start(myentity_iteration_state_t *state, enum myentity_type type);
extern void myentity_foreach_next(myentity_iteration_state_t *state);
extern struct myentity *myentity_foreach_cur(myentity_iteration_state_t *state);

#define MYENTITY_FOREACH_T(elem, state, type) for (myentity_foreach_start(state, type); (elem = myentity_foreach_cur(state)); myentity_foreach_next(state))
#define MYENTITY_FOREACH(elem, state) MYENTITY_FOREACH_T(elem, state, 0)

extern void myentity_stats(void (*cb)(const char *line, void *privdata), void *privdata);

/* chanacs */
extern unsigned int myentity_count_channels_with_flagset(struct myentity *mt, unsigned int flagset);
extern bool myentity_can_register_channel(struct myentity *mt);
extern bool myentity_allow_foundership(struct myentity *mt);

typedef struct {
	struct myentity *entity;
	const char *name;

	bool approval;
} hook_myentity_req_t;

#endif /* !ENTITY_H */
