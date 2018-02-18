/* entity-validation.h - entity validation
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef ENTITY_VALIDATION_H
#define ENTITY_VALIDATION_H

struct entity_chanacs_validation_vtable
{
	chanacs_t *(*match_entity)(chanacs_t *ca, struct myentity *mt);
	chanacs_t *(*match_user)(chanacs_t *ca, user_t *mt);

	bool (*can_register_channel)(struct myentity *mt);
	bool (*allow_foundership)(struct myentity *mt);
};

extern const struct entity_chanacs_validation_vtable *myentity_get_chanacs_validator(struct myentity *mt);

#endif
