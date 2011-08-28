/* entity-validation.h - entity validation
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef ENTITY_VALIDATION_H
#define ENTITY_VALIDATION_H

struct entity_chanacs_validation_vtable {
	chanacs_t *(*match_entity)(chanacs_t *ca, myentity_t *mt);
	chanacs_t *(*match_user)(chanacs_t *ca, user_t *mt);

	bool (*can_register_channel)(myentity_t *mt);
};

E entity_chanacs_validation_vtable_t *myentity_get_chanacs_validator(myentity_t *mt);

#endif
