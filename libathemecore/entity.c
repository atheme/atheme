/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * entities.c - entity tracking
 */

#include <atheme.h>
#include "internal.h"

static mowgli_patricia_t *entities = NULL;
static mowgli_patricia_t *entities_by_id = NULL;

static char last_entity_uid[IDLEN + 1];

void
init_entities(void)
{
	entities = mowgli_patricia_create(irccasecanon);
	entities_by_id = mowgli_patricia_create(noopcanon);
	memset(last_entity_uid, 'A', sizeof last_entity_uid);
}

void
myentity_set_last_uid(const char *last_id)
{
	(void) mowgli_strlcpy(last_entity_uid, last_id, sizeof last_entity_uid);
}

const char *
myentity_get_last_uid(void)
{
	return last_entity_uid;
}

const char *
myentity_alloc_uid(void)
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

void
myentity_put(struct myentity *mt)
{
	/* If the entity doesn't have an ID yet, allocate one */
	if (mt->id[0] == '\0')
		mowgli_strlcpy(mt->id, myentity_alloc_uid(), sizeof mt->id);

	mowgli_patricia_add(entities, mt->name, mt);
	mowgli_patricia_add(entities_by_id, mt->id, mt);
}

void
myentity_del(struct myentity *mt)
{
	mowgli_patricia_delete(entities, mt->name);
	mowgli_patricia_delete(entities_by_id, mt->id);
}

struct myentity *
myentity_find(const char *name)
{
	struct myentity *ent;
	struct hook_myentity_req req;

	return_val_if_fail(name != NULL, NULL);

	if ((ent = mowgli_patricia_retrieve(entities, name)) != NULL)
		return ent;

	req.name = name;
	req.entity = NULL;
	hook_call_myentity_find(&req);

	return req.entity;
}

struct myentity *
myentity_find_uid(const char *uid)
{
	return_val_if_fail(uid != NULL, NULL);

	return mowgli_patricia_retrieve(entities_by_id, uid);
}

struct myentity *
myentity_find_ext(const char *name)
{
	struct myentity *mt;

	return_val_if_fail(name != NULL, NULL);

	mt = entity(myuser_find_ext(name));
	if (mt != NULL)
		return mt;

	return myentity_find(name);
}

void
myentity_foreach_start(struct myentity_iteration_state *state, enum myentity_type type)
{
	struct myentity *e;

	state->type = type;
	mowgli_patricia_foreach_start(entities, &state->st);

	e = mowgli_patricia_foreach_cur(entities, &state->st);
	while (e && state->type != ENT_ANY && state->type != e->type)
	{
		mowgli_patricia_foreach_next(entities, &state->st);
		e = mowgli_patricia_foreach_cur(entities, &state->st);
	}
}

struct myentity *
myentity_foreach_cur(struct myentity_iteration_state *state)
{
	return mowgli_patricia_foreach_cur(entities, &state->st);
}

void
myentity_foreach_next(struct myentity_iteration_state *state)
{
	struct myentity *e;
	do {
		mowgli_patricia_foreach_next(entities, &state->st);
		e = mowgli_patricia_foreach_cur(entities, &state->st);
	} while (e && state->type != ENT_ANY && state->type != e->type);
}

void
myentity_foreach(int (*cb)(struct myentity *mt, void *privdata), void *privdata)
{
	myentity_foreach_t(ENT_ANY, cb, privdata);
}

void
myentity_foreach_t(enum myentity_type type, int (*cb)(struct myentity *mt, void *privdata), void *privdata)
{
	struct myentity_iteration_state state;
	struct myentity *mt;
	MYENTITY_FOREACH_T(mt, &state, type)
	{
		if (cb(mt, privdata))
			return;
	}
}

void
myentity_stats(void (*cb)(const char *line, void *privdata), void *privdata)
{
	mowgli_patricia_stats(entities, cb, privdata);
}

/* validation */
static bool
linear_match_entity(struct myentity *self, struct myentity *mt)
{
	return self == mt;
}

static bool
linear_can_register_channel(struct myentity *mt)
{
	struct myuser *mu;

	if ((mu = user(mt)) == NULL)
		return false;

	if (mu->flags & MU_REGNOLIMIT)
		return true;

	return has_priv_myuser(mu, PRIV_REG_NOLIMIT);
}

static bool
linear_allow_foundership(struct myentity *mt)
{
	struct myuser *mu;

	/* avoid workaround for restricted users where foundership is set on the user after registration. */
	if ((mu = user(mt)) != NULL)
	{
		struct metadata *md;

		md = metadata_find(mu, "private:restrict:setter");
		if (md != NULL)
			return false;
	}

	return true;
}

static const struct entity_vtable linear_validate = {
	.match_entity = linear_match_entity,
	.can_register_channel = linear_can_register_channel,
	.allow_foundership = linear_allow_foundership,
};

const struct entity_vtable *
myentity_get_vtable(struct myentity *mt)
{
	return mt->vtable != NULL ? mt->vtable : &linear_validate;
}

/* chanacs */
unsigned int
myentity_count_channels_with_flagset(struct myentity *mt, unsigned int flagset)
{
	mowgli_node_t *n;
	struct chanacs *ca;
	unsigned int count = 0;

	MOWGLI_ITER_FOREACH(n, mt->chanacs.head)
	{
		ca = n->data;
		if (ca->level & flagset)
			count++;
	}

	return count;
}

bool
myentity_can_register_channel(struct myentity *mt)
{
	const struct entity_vtable *vt;

	return_val_if_fail(mt != NULL, false);

	vt = myentity_get_vtable(mt);
	if (vt->can_register_channel(mt))
		return true;

	return (myentity_count_channels_with_flagset(mt, CA_FOUNDER) < chansvs.maxchans);
}

bool
myentity_allow_foundership(struct myentity *mt)
{
	const struct entity_vtable *vt;

	return_val_if_fail(mt != NULL, false);

	vt = myentity_get_vtable(mt);
	if (vt->allow_foundership)
		return vt->allow_foundership(mt);

	return false;
}
