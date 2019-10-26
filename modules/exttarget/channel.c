/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 */

#include <atheme.h>
#include "exttarget.h"

struct this_exttarget
{
	struct myentity parent;
	stringref channel;
};

static mowgli_heap_t *channel_ext_heap = NULL;
static mowgli_patricia_t *channel_exttarget_tree = NULL;
static mowgli_patricia_t **exttarget_tree = NULL;

static bool
channel_ext_match_user(struct myentity *self, struct user *u)
{
	struct this_exttarget *ent;
	mowgli_node_t *n;

	ent = (struct this_exttarget *) self;
	MOWGLI_LIST_FOREACH(n, u->channels.head)
	{
		struct chanuser *cu = n->data;

		if (!irccasecmp(cu->chan->name, ent->channel))
			return true;
	}

	return false;
}

static bool
channel_ext_match_entity(struct myentity *self, struct myentity *mt)
{
	return self == mt;
}

static bool
channel_ext_can_register_channel(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static bool
channel_ext_allow_foundership(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static void
channel_ext_delete(struct this_exttarget *e)
{
	return_if_fail(e != NULL);

	mowgli_patricia_delete(channel_exttarget_tree, e->channel);
	strshare_unref(e->channel);
	strshare_unref(entity(e)->name);

	mowgli_heap_free(channel_ext_heap, e);
}

static struct myentity *
channel_validate_f(const char *param)
{
	static const struct entity_vtable channel_ext_vtable = {
		.match_entity = channel_ext_match_entity,
		.match_user = channel_ext_match_user,
		.can_register_channel = channel_ext_can_register_channel,
		.allow_foundership = channel_ext_allow_foundership,
	};

	char *name;
	struct this_exttarget *ext;
	size_t namelen;

	if (param == NULL)
		return NULL;

	if (*param == '\0')
		return NULL;

	// if we already have an object, return it from our tree.
	if ((ext = mowgli_patricia_retrieve(channel_exttarget_tree, param)) != NULL)
		return entity(ext);

	ext = mowgli_heap_alloc(channel_ext_heap);
	ext->channel = strshare_get(param);

	// name the entity... $channel:param
#define NAMEPREFIX "$channel:"
	namelen = sizeof NAMEPREFIX + strlen(param);

	name = smalloc(namelen);
	memcpy(name, NAMEPREFIX, sizeof NAMEPREFIX - 1);
	memcpy(name + sizeof NAMEPREFIX - 1, param, namelen - sizeof NAMEPREFIX + 1);

	entity(ext)->name = strshare_get(name);
	sfree(name);
#undef NAMEPREFIX

	// hook up the entity's vtable.
	entity(ext)->vtable = &channel_ext_vtable;
	entity(ext)->type = ENT_EXTTARGET;

	// initialize the object.
	atheme_object_init(atheme_object(ext), entity(ext)->name, (atheme_object_destructor_fn) channel_ext_delete);

	// add the object to the exttarget tree.
	mowgli_patricia_add(channel_exttarget_tree, ext->channel, ext);

	// return the object as initially unowned by sinking the reference count.
	return atheme_object_sink_ref(ext);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree")

	mowgli_patricia_add(*exttarget_tree, "channel", channel_validate_f);

	// since we are dealing with channel names, we use irccasecanon.
	channel_exttarget_tree = mowgli_patricia_create(irccasecanon);
	channel_ext_heap = mowgli_heap_create(sizeof(struct this_exttarget), 32, BH_LAZY);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_heap_destroy(channel_ext_heap);
	mowgli_patricia_delete(*exttarget_tree, "channel");
	mowgli_patricia_destroy(channel_exttarget_tree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("exttarget/channel", MODULE_UNLOAD_CAPABILITY_OK)
