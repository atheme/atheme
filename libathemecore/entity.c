/* entities.c - entity tracking
 * Copyright (C) 2010 Atheme Development Group
 */

#include "atheme.h"

static mowgli_patricia_t *entities;
static mowgli_patricia_t *entities_by_id;
static char last_entity_uid[IDLEN];

void init_entities(void)
{
	entities = mowgli_patricia_create(irccasecanon);
	entities_by_id = mowgli_patricia_create(noopcanon);
	memset(last_entity_uid, 'A', sizeof last_entity_uid);
}

void myentity_set_last_uid(const char *last_id)
{
	mowgli_strlcpy(last_entity_uid, last_id, sizeof last_entity_uid);
	last_entity_uid[IDLEN-1] = '\0';
}

const char *myentity_get_last_uid(void)
{
	return last_entity_uid;
}

const char *myentity_alloc_uid(void)
{
	int i;

	for(i = 8; i > 3; i--)
	{
		if(last_entity_uid[i] == 'Z')
		{
			last_entity_uid[i] = '0';
			return last_entity_uid;
		}
		else if(last_entity_uid[i] != '9')
		{
			last_entity_uid[i]++;
			return last_entity_uid;
		}
		else
			last_entity_uid[i] = 'A';
	}

	/* if this next if() triggers, we're fucked. */
	if(last_entity_uid[3] == 'Z')
	{
		last_entity_uid[i] = 'A';
		slog(LG_ERROR, "Out of entity UIDs!");
		wallops("Out of entity UIDs. This is a Bad Thing.");
		wallops("You should probably do something about this.");
	}
	else
		last_entity_uid[3]++;

	return last_entity_uid;
}

void myentity_put(myentity_t *mt)
{
	/* If the entity doesn't have an ID yet, allocate one */
	if (mt->id[0] == '\0')
		mowgli_strlcpy(mt->id, myentity_alloc_uid(), sizeof mt->id);

	mowgli_patricia_add(entities, mt->name, mt);
	mowgli_patricia_add(entities_by_id, mt->id, mt);
}

void myentity_del(myentity_t *mt)
{
	mowgli_patricia_delete(entities, mt->name);
	mowgli_patricia_delete(entities_by_id, mt->id);
}

myentity_t *myentity_find(const char *name)
{
	myentity_t *ent;
	hook_myentity_req_t req;

	return_val_if_fail(name != NULL, NULL);

	if ((ent = mowgli_patricia_retrieve(entities, name)) != NULL)
		return ent;

	req.name = name;
	req.entity = NULL;
	hook_call_myentity_find(&req);

	return req.entity;
}

myentity_t *myentity_find_uid(const char *uid)
{
	return_val_if_fail(uid != NULL, NULL);

	return mowgli_patricia_retrieve(entities_by_id, uid);
}

myentity_t *myentity_find_ext(const char *name)
{
	myentity_t *mt;

	return_val_if_fail(name != NULL, NULL);

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

	return (myentity_count_channels_with_flagset(mt, CA_FOUNDER) < chansvs.maxchans);
}

