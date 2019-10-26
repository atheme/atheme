/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011 William Pitcock <nenolod@dereferenced.org>
 */

#include <atheme.h>
#include "exttarget.h"

static mowgli_patricia_t **exttarget_tree = NULL;

static bool
registered_ext_match_user(struct myentity ATHEME_VATTR_UNUSED *self, struct user *u)
{
	return u->myuser != NULL && !(u->myuser->flags & MU_WAITAUTH);
}

static bool
registered_ext_match_entity(struct myentity *self, struct myentity *mt)
{
	return self == mt;
}

static bool
registered_ext_can_register_channel(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static bool
registered_ext_allow_foundership(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static const struct entity_vtable registered_ext_vtable = {
	.match_entity = registered_ext_match_entity,
	.match_user = registered_ext_match_user,
	.can_register_channel = registered_ext_can_register_channel,
	.allow_foundership = registered_ext_allow_foundership,
};

static struct myentity registered_ext_entity = {
	.name = "$registered",
	.type = ENT_EXTTARGET,
	.vtable = &registered_ext_vtable,
};

static struct myentity *
registered_validate_f(const char ATHEME_VATTR_UNUSED *param)
{
	return &registered_ext_entity;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree")

	mowgli_patricia_add(*exttarget_tree, "registered", registered_validate_f);

	atheme_object_init(atheme_object(&registered_ext_entity), "$registered", NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_patricia_delete(*exttarget_tree, "registered");
}

SIMPLE_DECLARE_MODULE_V1("exttarget/registered", MODULE_UNLOAD_CAPABILITY_OK)
