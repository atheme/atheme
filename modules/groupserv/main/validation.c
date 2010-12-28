/* groupserv - group services.
 * Copyright (c) 2010 Atheme Development Group
 */

#include "groupserv.h"

static chanacs_t *mygroup_chanacs_match_entity(chanacs_t *ca, myentity_t *mt)
{
	mygroup_t *mg;

	mg = group(ca->entity);

	return_val_if_fail(mg != NULL, NULL);
	
	if (!isuser(mt))
		return NULL;

	return groupacs_find(mg, user(mt), GA_CHANACS) != NULL ? ca : NULL;
}

static bool mygroup_can_register_channel(myentity_t *mt)
{
	mygroup_t *mg;

	mg = group(mt);

	return_val_if_fail(mg != NULL, false);

	if (mg->flags & MG_REGNOLIMIT)
		return true;

	return false;
}

static entity_chanacs_validation_vtable_t mygroup_chanacs_validate = {
	.match_entity = mygroup_chanacs_match_entity,
	.can_register_channel = mygroup_can_register_channel,
};

void mygroup_set_chanacs_validator(myentity_t *mt) {
	mt->chanacs_validate = &mygroup_chanacs_validate;
}
