/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2007 William Pitcock <nenolod -at- atheme.org>
 *
 * Object management.
 */

#ifndef ATHEME_INC_OBJECT_H
#define ATHEME_INC_OBJECT_H 1

#include <atheme/common.h>
#include <atheme/stdheaders.h>

struct metadata
{
	stringref       name;
	char *          value;
};

typedef void (*atheme_object_destructor_fn)(void *);

struct atheme_object
{
	int                             refcount;
	atheme_object_destructor_fn     destructor;
	mowgli_patricia_t *             metadata;
	mowgli_patricia_t *             privatedata;
#ifdef OBJECT_DEBUG
	mowgli_node_t                   dnode;
#endif
};

void init_metadata(void);

void atheme_object_init(struct atheme_object *, const char *name, atheme_object_destructor_fn destructor);
void *atheme_object_ref(void *);
void *atheme_object_sink_ref(void *);
void atheme_object_unref(void *);
void atheme_object_dispose(void *);

struct metadata *metadata_add(void *target, const char *name, const char *value);
void metadata_delete(void *target, const char *name);
struct metadata *metadata_find(void *target, const char *name);
void metadata_delete_all(void *target);

void *privatedata_get(void *target, const char *key);
void privatedata_set(void *target, const char *key, void *data);
void *privatedata_delete(void *target, const char *key);

#ifdef OBJECT_DEBUG
extern mowgli_list_t object_list;
#endif

#define atheme_object(x) ((struct atheme_object *) x)

#endif /* !ATHEME_INC_OBJECT_H */
