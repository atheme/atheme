/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 Atheme Project (http://atheme.org/)
 *
 * entity-validation.h - entity validation
 */

#ifndef ATHEME_INC_ENTITY_VALIDATION_H
#define ATHEME_INC_ENTITY_VALIDATION_H 1

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

struct entity_vtable
{
	bool                  (*match_entity)(struct myentity *self, struct myentity *mt);
	bool                  (*match_user)(struct myentity *self, struct user *mt);
	bool                  (*can_register_channel)(struct myentity *self);
	bool                  (*allow_foundership)(struct myentity *self);
};

const struct entity_vtable *myentity_get_vtable(struct myentity *mt);

#endif /* !ATHEME_INC_ENTITY_VALIDATION_H */
