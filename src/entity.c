/* entities.c - entity tracking
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"

static mowgli_patricia_t *entities;

void init_entities(void)
{
	entities = mowgli_patricia_create(irccasecanon);
}

void myentity_put(myentity_t *me)
{
	mowgli_patricia_add(entities, me->name, me);
}

void myentity_del(myentity_t *me)
{
	mowgli_patricia_delete(entities, me->name);
}

myentity_t *myentity_find(const char *name)
{
	return mowgli_patricia_retrieve(entities, name);
}

void myentity_foreach_start(myentity_iteration_state_t *state, myentity_type_t type)
{
	state->type = type;
	mowgli_patricia_foreach_start(entities, &state->st);
}

myentity_t *myentity_foreach_cur(myentity_iteration_state_t *state)
{
#ifdef NOTYET
	myentity_t *mt;

	mt = mowgli_patricia_foreach_cur(entities, &state->st);
	if (mt == NULL)
		return NULL;

	if (state->type == ENT_ANY)
		return mt;

	return state->type == mt->type ? mt : NULL;
#else
	return mowgli_patricia_foreach_cur(entities, &state->st);
#endif
}

void myentity_foreach_next(myentity_iteration_state_t *state)
{
	myentity_t *e;
	do {
		mowgli_patricia_foreach_next(entities, &state->st);
		e = mowgli_patricia_foreach_cur(entities, &state->st);
	} while (e && state->type != ENT_ANY && state->type != e->type);
}

void myentity_foreach(int (*cb)(myentity_t *me, void *privdata), void *privdata)
{
	myentity_foreach_t(ENT_ANY, cb, privdata);
}

void myentity_foreach_t(myentity_type_t type, int (*cb)(myentity_t *me, void *privdata), void *privdata)
{
	myentity_iteration_state_t state;
	myentity_t *me;
	MYENTITY_FOREACH_T(me, &state, type)
	{
		if (cb(me, privdata))
			return;
	}
}

void myentity_stats(void (*cb)(const char *line, void *privdata), void *privdata)
{
	mowgli_patricia_stats(entities, cb, privdata);
}

/* validation */
static chanacs_t *linear_chanacs_match_entity(chanacs_t *ca, myentity_t *mt)
{
	return ca->entity == mt ? ca : NULL;
}

entity_chanacs_validation_vtable_t linear_chanacs_validate = {
	.match_entity = linear_chanacs_match_entity,
};

entity_chanacs_validation_vtable_t *myentity_get_chanacs_validator(myentity_t *mt)
{
	return mt->chanacs_validate != NULL ? mt->chanacs_validate : &linear_chanacs_validate;
}
