/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * Rights to this code are as documented in doc/LICENSE.
 */

#include "atheme.h"
#include "exttarget.h"

static mowgli_patricia_t **exttarget_tree = NULL;

static struct chanacs *
dummy_match_user(struct chanacs *ca, struct user *u)
{
	if (is_ircop(u))
		return ca;

	return NULL;
}

static struct chanacs *
dummy_match_entity(struct chanacs *ca, struct myentity *mt)
{
	if (ca->entity == mt)
		return ca;

	return NULL;
}

static bool
dummy_can_register_channel(struct myentity *mt)
{
	return false;
}

static bool
dummy_allow_foundership(struct myentity *mt)
{
	return false;
}

static const struct entity_chanacs_validation_vtable dummy_validate = {
	.match_entity = dummy_match_entity,
	.match_user = dummy_match_user,
	.can_register_channel = dummy_can_register_channel,
	.allow_foundership = dummy_allow_foundership,
};

static struct myentity dummy_entity = {
	.name = "$oper",
	.type = ENT_EXTTARGET,
	.chanacs_validate = &dummy_validate,
};

static struct myentity *
oper_validate_f(const char *param)
{
	return &dummy_entity;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree");

	mowgli_patricia_add(*exttarget_tree, "oper", oper_validate_f);

	atheme_object_init(atheme_object(&dummy_entity), "$oper", NULL);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_patricia_delete(*exttarget_tree, "oper");
}

SIMPLE_DECLARE_MODULE_V1("exttarget/oper", MODULE_UNLOAD_CAPABILITY_OK)
