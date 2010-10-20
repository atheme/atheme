/* entities.c - entity tracking
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"

static mowgli_patricia_t *entities;

void init_entities(void)
{
	entities = mowgli_patricia_create(irccasecanon);
}

void myentity_put(myentity_t *mt)
{
	mowgli_patricia_add(entities, mt->name, mt);
}

void myentity_del(myentity_t *mt)
{
	mowgli_patricia_delete(entities, mt->name);
}

myentity_t *myentity_find(const char *name)
{
	return mowgli_patricia_retrieve(entities, name);
}

myentity_t *myentity_find_ext(const char *name)
{
	myentity_t *mt;

	mt = entity(myuser_find_ext(name));
	if (mt != NULL)
		return mt;

	return myentity_find(name);
}

void myentity_foreach_start(myentity_iteration_state_t *state, myentity_type_t type)
{
	myentity_t *e;

	state->type = type;
	mowgli_patricia_foreach_start(entities, &state->st);

	e = mowgli_patricia_foreach_cur(entities, &state->st);
	while (e && state->type != ENT_ANY && state->type != e->type)
	{
		mowgli_patricia_foreach_next(entities, &state->st);
		e = mowgli_patricia_foreach_cur(entities, &state->st);
	} 
}

myentity_t *myentity_foreach_cur(myentity_iteration_state_t *state)
{
	return mowgli_patricia_foreach_cur(entities, &state->st);
}

void myentity_foreach_next(myentity_iteration_state_t *state)
{
	myentity_t *e;
	do {
		mowgli_patricia_foreach_next(entities, &state->st);
		e = mowgli_patricia_foreach_cur(entities, &state->st);
	} while (e && state->type != ENT_ANY && state->type != e->type);
}

void myentity_foreach(int (*cb)(myentity_t *mt, void *privdata), void *privdata)
{
	myentity_foreach_t(ENT_ANY, cb, privdata);
}

void myentity_foreach_t(myentity_type_t type, int (*cb)(myentity_t *mt, void *privdata), void *privdata)
{
	myentity_iteration_state_t state;
	myentity_t *mt;
	MYENTITY_FOREACH_T(mt, &state, type)
	{
		if (cb(mt, privdata))
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

static bool linear_can_register_channel(myentity_t *mt)
{
	myuser_t *mu;

	if ((mu = user(mt)) == NULL)
		return false;

	if (mu->flags & MU_REGNOLIMIT)
		return true;

	return has_priv_myuser(mu, PRIV_REG_NOLIMIT);
}

entity_chanacs_validation_vtable_t linear_chanacs_validate = {
	.match_entity = linear_chanacs_match_entity,
	.can_register_channel = linear_can_register_channel,
};

entity_chanacs_validation_vtable_t *myentity_get_chanacs_validator(myentity_t *mt)
{
	return mt->chanacs_validate != NULL ? mt->chanacs_validate : &linear_chanacs_validate;
}

/* chanacs */
unsigned int myentity_count_channels_with_flagset(myentity_t *mt, unsigned int flagset)
{
	mowgli_node_t *n;
	chanacs_t *ca;
	unsigned int count = 0;

	MOWGLI_ITER_FOREACH(n, mt->chanacs.head)
	{
		ca = n->data;
		if (ca->level & flagset)
			count++;
	}

	return count;
}

bool myentity_can_register_channel(myentity_t *mt)
{
	entity_chanacs_validation_vtable_t *vt;

	return_val_if_fail(mt != NULL, false);

	vt = myentity_get_chanacs_validator(mt);
	if (vt->can_register_channel(mt))
		return true;

	return (myentity_count_channels_with_flagset(mt, CA_FOUNDER) < me.maxchans);
}

