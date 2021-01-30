/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * groupserv - group services.
 */

#include <atheme.h>
#include "groupserv_main.h"

static bool
mygroup_match_entity(struct myentity *self, struct myentity *mt)
{
	struct mygroup *mg;

	mg = group(self);

	return_val_if_fail(mg != NULL, false);

	if (!isuser(mt))
		return false;

	return groupacs_find(mg, mt, GA_CHANACS, true) != NULL;
}

static bool
mygroup_match_user(struct myentity *self, struct user *u)
{
	return_val_if_fail(u != NULL, false);
	return mygroup_match_entity(self, entity(u->myuser));
}

static bool
mygroup_can_register_channel(struct myentity *mt)
{
	struct mygroup *mg;

	mg = group(mt);

	return_val_if_fail(mg != NULL, false);

	if (mg->flags & MG_REGNOLIMIT)
		return true;

	return false;
}

static bool
mygroup_allow_foundership(struct myentity ATHEME_VATTR_UNUSED *mt)
{
	return true;
}

void
mygroup_set_entity_vtable(struct myentity *mt)
{
	static const struct entity_vtable mygroup_vtable = {
		.match_user = mygroup_match_user,
		.match_entity = mygroup_match_entity,
		.can_register_channel = mygroup_can_register_channel,
		.allow_foundership = mygroup_allow_foundership,
	};

	mt->vtable = &mygroup_vtable;
}
