/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * Rights to this code are as documented in doc/LICENSE.
 * 
 * $chanacs:#channel gives flags to users in /cs flags #channel if
 *   the user has flags in the $chanacs:#channel
 */

#include "atheme.h"
#include "exttarget.h"

DECLARE_MODULE_V1
(
	"exttarget/chanacs", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static mowgli_patricia_t **exttarget_tree = NULL;

typedef struct {
	myentity_t parent;
	stringref channel;
	int checking;
} chanacs_exttarget_t;

static chanacs_t *chanacs_ext_match_user(chanacs_t *ca, user_t *u)
{
	chanacs_exttarget_t *ent;
	mychan_t *mc;
	unsigned int flags;

	ent = (chanacs_exttarget_t *) ca->entity;

	if (ent->checking > 4) /* arbitrary recursion limit? */
		return NULL;

	if (!(mc = mychan_find(ent->channel)))
		return NULL;

	ent->checking++;
	flags = chanacs_user_flags(mc, u);
	ent->checking--;

	if (flags & CA_AKICK)
		return NULL;

	if (flags)
		return ca;

	return NULL;
}

static chanacs_t *chanacs_ext_match_entity(chanacs_t *ca, myentity_t *mt)
{
	if (ca->entity == mt)
		return ca;

	return NULL;
}

static bool chanacs_ext_can_register_channel(myentity_t *mt)
{
	return false;
}

static entity_chanacs_validation_vtable_t chanacs_ext_validate = {
	.match_entity = chanacs_ext_match_entity,
	.match_user = chanacs_ext_match_user,
	.can_register_channel = chanacs_ext_can_register_channel,
};

static mowgli_heap_t *chanacs_ext_heap = NULL;
static mowgli_patricia_t *chanacs_exttarget_tree = NULL;

static void chanacs_ext_delete(chanacs_exttarget_t *e)
{
	return_if_fail(e != NULL);

	mowgli_patricia_delete(chanacs_exttarget_tree, e->channel);
	strshare_unref(e->channel);
	strshare_unref(entity(e)->name);

	mowgli_heap_free(chanacs_ext_heap, e);
}

static myentity_t *chanacs_validate_f(const char *param)
{
	char *name;
	chanacs_exttarget_t *ext;
	size_t namelen;

	if (param == NULL)
		return NULL;

	if (*param == '\0')
		return NULL;

	/* if we already have an object, return it from our tree. */
	if ((ext = mowgli_patricia_retrieve(chanacs_exttarget_tree, param)) != NULL)
		return entity(ext);

	ext = mowgli_heap_alloc(chanacs_ext_heap);
	ext->channel = strshare_get(param);
	ext->checking = 0;

	/* name the entity... $chanacs:param */
#define NAMEPREFIX "$chanacs:"
	namelen = sizeof NAMEPREFIX + strlen(param);

	name = smalloc(namelen);
	memcpy(name, NAMEPREFIX, sizeof NAMEPREFIX - 1);
	memcpy(name + sizeof NAMEPREFIX - 1, param, namelen - sizeof NAMEPREFIX + 1);

	entity(ext)->name = strshare_get(name);
	free(name);
#undef NAMEPREFIX

	/* hook up the entity's validation table. */
	entity(ext)->chanacs_validate = &chanacs_ext_validate;
	entity(ext)->type = ENT_EXTTARGET;

	/* initialize the object. */
	object_init(object(ext), entity(ext)->name, (destructor_t) chanacs_ext_delete);

	/* add the object to the exttarget tree */
	mowgli_patricia_add(chanacs_exttarget_tree, ext->channel, ext);

	/* return the object as initially unowned by sinking the reference count. */
	return object_sink_ref(ext);
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree");

	mowgli_patricia_add(*exttarget_tree, "chanacs", chanacs_validate_f);

	/* since we are dealing with channel names, we use irccasecanon. */
	chanacs_exttarget_tree = mowgli_patricia_create(irccasecanon);
	chanacs_ext_heap = mowgli_heap_create(sizeof(chanacs_exttarget_t), 32, BH_LAZY);
}

void _moddeinit(module_unload_intent_t intent)
{
	mowgli_heap_destroy(chanacs_ext_heap);
	mowgli_patricia_delete(*exttarget_tree, "chanacs");
	mowgli_patricia_destroy(chanacs_exttarget_tree, NULL, NULL);
}
