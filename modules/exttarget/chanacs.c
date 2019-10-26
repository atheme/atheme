/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * $chanacs:#channel gives flags to users in /cs flags #channel if
 *   the user has flags in the $chanacs:#channel
 */

#include <atheme.h>
#include "exttarget.h"

struct this_exttarget
{
	struct myentity parent;
	stringref channel;
	int checking;
};

static mowgli_heap_t *chanacs_ext_heap = NULL;
static mowgli_patricia_t *chanacs_exttarget_tree = NULL;
static mowgli_patricia_t **exttarget_tree = NULL;

static bool
chanacs_ext_match_user(struct myentity *self, struct user *u)
{
	struct this_exttarget *ent;
	struct mychan *mc;
	unsigned int flags;

	ent = (struct this_exttarget *) self;

	if (ent->checking > 4) // arbitrary recursion limit?
		return false;

	if (!(mc = mychan_find(ent->channel)))
		return false;

	ent->checking++;
	flags = chanacs_user_flags(mc, u);
	ent->checking--;

	if (flags & CA_AKICK)
		return false;

	if (flags)
		return true;

	return false;
}

static bool
chanacs_ext_match_entity(struct myentity *self, struct myentity *mt)
{
	return self == mt;
}

static bool
chanacs_ext_can_register_channel(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static bool
chanacs_ext_allow_foundership(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static void
chanacs_ext_delete(struct this_exttarget *e)
{
	return_if_fail(e != NULL);

	mowgli_patricia_delete(chanacs_exttarget_tree, e->channel);
	strshare_unref(e->channel);
	strshare_unref(entity(e)->name);

	mowgli_heap_free(chanacs_ext_heap, e);
}

static struct myentity *
chanacs_validate_f(const char *param)
{
	static const struct entity_vtable chanacs_ext_vtable = {
		.match_entity = chanacs_ext_match_entity,
		.match_user = chanacs_ext_match_user,
		.can_register_channel = chanacs_ext_can_register_channel,
		.allow_foundership = chanacs_ext_allow_foundership,
	};

	char *name;
	struct this_exttarget *ext;
	size_t namelen;

	if (param == NULL)
		return NULL;

	if (*param == '\0')
		return NULL;

	// if we already have an object, return it from our tree.
	if ((ext = mowgli_patricia_retrieve(chanacs_exttarget_tree, param)) != NULL)
		return entity(ext);

	ext = mowgli_heap_alloc(chanacs_ext_heap);
	ext->channel = strshare_get(param);
	ext->checking = 0;

	// name the entity... $chanacs:param
#define NAMEPREFIX "$chanacs:"
	namelen = sizeof NAMEPREFIX + strlen(param);

	name = smalloc(namelen);
	memcpy(name, NAMEPREFIX, sizeof NAMEPREFIX - 1);
	memcpy(name + sizeof NAMEPREFIX - 1, param, namelen - sizeof NAMEPREFIX + 1);

	entity(ext)->name = strshare_get(name);
	sfree(name);
#undef NAMEPREFIX

	// hook up the entity's vtable
	entity(ext)->vtable = &chanacs_ext_vtable;
	entity(ext)->type = ENT_EXTTARGET;

	// initialize the object.
	atheme_object_init(atheme_object(ext), entity(ext)->name, (atheme_object_destructor_fn) chanacs_ext_delete);

	// add the object to the exttarget tree
	mowgli_patricia_add(chanacs_exttarget_tree, ext->channel, ext);

	// return the object as initially unowned by sinking the reference count.
	return atheme_object_sink_ref(ext);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree")

	mowgli_patricia_add(*exttarget_tree, "chanacs", chanacs_validate_f);

	// since we are dealing with channel names, we use irccasecanon.
	chanacs_exttarget_tree = mowgli_patricia_create(irccasecanon);
	chanacs_ext_heap = mowgli_heap_create(sizeof(struct this_exttarget), 32, BH_LAZY);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_heap_destroy(chanacs_ext_heap);
	mowgli_patricia_delete(*exttarget_tree, "chanacs");
	mowgli_patricia_destroy(chanacs_exttarget_tree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("exttarget/chanacs", MODULE_UNLOAD_CAPABILITY_OK)
