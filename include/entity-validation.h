/* entity-validation.h - entity validation
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef ENTITY_VALIDATION_H
#define ENTITY_VALIDATION_H

struct entity_chanacs_validation_vtable
{
	struct chanacs *(*match_entity)(struct chanacs *ca, struct myentity *mt);
	struct chanacs *(*match_user)(struct chanacs *ca, struct user *mt);

	bool (*can_register_channel)(struct myentity *mt);
	bool (*allow_foundership)(struct myentity *mt);
};

const struct entity_chanacs_validation_vtable *myentity_get_chanacs_validator(struct myentity *mt);

#endif
