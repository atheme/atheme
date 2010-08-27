/* entity-validation.h - entity validation
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef ENTITY_VALIDATION_H
#define ENTITY_VALIDATION_H

typedef struct {
	chanacs_t *(*match_entity)(chanacs_t *ca, myentity_t *mt);
} entity_chanacs_validation_vtable_t;

#endif
