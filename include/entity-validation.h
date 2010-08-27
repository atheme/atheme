/* entity-validation.h - entity validation
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef ENTITY_VALIDATION_H
#define ENTITY_VALIDATION_H

typedef struct {
	chanacs_t *(*match_user)(chanacs_t *ca, user_t *u);
	chanacs_t *(*match_myuser)(chanacs_t *ca, myuser_t *mu);
} entity_chanacs_validation_vtable_t;

#endif
