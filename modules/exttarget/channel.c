/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * Rights to this code are as documented in doc/LICENSE.
 */

#include "atheme.h"
#include "exttarget.h"

DECLARE_MODULE_V1
(
	"exttarget/channel", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static mowgli_patricia_t **exttarget_tree = NULL;

typedef struct {
	myentity_t parent;
	stringref channel;
} channel_exttarget_t;

static chanacs_t *channel_ext_match_user(chanacs_t *ca, user_t *u)
{
	channel_exttarget_t *ent;
	mowgli_node_t *n;

	ent = (channel_exttarget_t *) ca->entity;
	MOWGLI_LIST_FOREACH(n, u->channels.head)
	{
		chanuser_t *cu = n->data;

		if (!irccasecmp(cu->chan->name, ent->channel))
			return ca;
	}

	return NULL;
}

static chanacs_t *channel_ext_match_entity(chanacs_t *ca, myentity_t *mt)
{
	if (ca->entity == mt)
		return ca;

	return NULL;
}

static bool channel_ext_can_register_channel(myentity_t *mt)
{
	return false;
}

static entity_chanacs_validation_vtable_t channel_ext_validate = {
	.match_entity = channel_ext_match_entity,
	.match_user = channel_ext_match_user,
	.can_register_channel = channel_ext_can_register_channel,
};

static mowgli_heap_t *channel_ext_heap = NULL;
static mowgli_patricia_t *channel_exttarget_tree = NULL;

static void channel_ext_delete(channel_exttarget_t *e)
{
	return_if_fail(e != NULL);

	mowgli_patricia_delete(channel_exttarget_tree, e->channel);
	strshare_unref(e->channel);
	strshare_unref(entity(e)->name);

	mowgli_heap_free(channel_ext_heap, e);
}

static myentity_t *channel_validate_f(const char *param)
{
	char *name;
	channel_exttarget_t *ext;
	size_t namelen;

	if (param == NULL)
		return NULL;

	if (*param == '\0')
		return NULL;

	/* if we already have an object, return it from our tree. */
	if ((ext = mowgli_patricia_retrieve(channel_exttarget_tree, param)) != NULL)
		return entity(ext);

	ext = mowgli_heap_alloc(channel_ext_heap);
	ext->channel = strshare_get(param);

	/* name the entity... $channel:param */
#define NAMEPREFIX "$channel:"
	namelen = sizeof NAMEPREFIX + strlen(param);

	name = smalloc(namelen);
	memcpy(name, NAMEPREFIX, sizeof NAMEPREFIX - 1);
	memcpy(name + sizeof NAMEPREFIX - 1, param, namelen - sizeof NAMEPREFIX + 1);

	entity(ext)->name = strshare_get(name);
	free(name);
#undef NAMEPREFIX

	/* hook up the entity's validation table. */
	entity(ext)->chanacs_validate = &channel_ext_validate;
	entity(ext)->type = ENT_EXTTARGET;

	/* initialize the object. */
	object_init(object(ext), entity(ext)->name, (destructor_t) channel_ext_delete);

	/* add the object to the exttarget tree. */
	mowgli_patricia_add(channel_exttarget_tree, ext->channel, ext);

	/* return the object as initially unowned by sinking the reference count. */
	return object_sink_ref(ext);
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree");

	mowgli_patricia_add(*exttarget_tree, "channel", channel_validate_f);

	/* since we are dealing with channel names, we use irccasecanon. */
	channel_exttarget_tree = mowgli_patricia_create(irccasecanon);
	channel_ext_heap = mowgli_heap_create(sizeof(channel_exttarget_t), 32, BH_LAZY);
}

void _moddeinit(module_unload_intent_t intent)
{
	mowgli_heap_destroy(channel_ext_heap);
	mowgli_patricia_delete(*exttarget_tree, "channel");
	mowgli_patricia_destroy(channel_exttarget_tree, NULL, NULL);
}
