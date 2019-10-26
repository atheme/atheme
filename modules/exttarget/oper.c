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
oper_ext_match_user(struct myentity ATHEME_VATTR_UNUSED *self, struct user *u)
{
	return is_ircop(u);
}

static bool
oper_ext_match_entity(struct myentity *self, struct myentity *mt)
{
	return self == mt;
}

static bool
oper_ext_can_register_channel(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static bool
oper_ext_allow_foundership(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return false;
}

static const struct entity_vtable oper_ext_vtable = {
	.match_entity = oper_ext_match_entity,
	.match_user = oper_ext_match_user,
	.can_register_channel = oper_ext_can_register_channel,
	.allow_foundership = oper_ext_allow_foundership,
};

static struct myentity oper_ext_entity = {
	.name = "$oper",
	.type = ENT_EXTTARGET,
	.vtable = &oper_ext_vtable,
};

static struct myentity *
oper_validate_f(const char ATHEME_VATTR_UNUSED *param)
{
	return &oper_ext_entity;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree")

	mowgli_patricia_add(*exttarget_tree, "oper", oper_validate_f);

	atheme_object_init(atheme_object(&oper_ext_entity), "$oper", NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_patricia_delete(*exttarget_tree, "oper");
}

SIMPLE_DECLARE_MODULE_V1("exttarget/oper", MODULE_UNLOAD_CAPABILITY_OK)
