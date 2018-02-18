/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 */

#ifndef ATHEME_OBJECT_H
#define ATHEME_OBJECT_H

struct metadata
{
	stringref name;
	char *value;
};

typedef void (*destructor_t)(void *);

struct atheme_object
{
	int refcount;
	destructor_t destructor;
	mowgli_patricia_t *metadata;
	mowgli_patricia_t *privatedata;
#ifdef OBJECT_DEBUG
	mowgli_node_t dnode;
#endif
};

extern void init_metadata(void);

extern void atheme_object_init(struct atheme_object *, const char *name, destructor_t destructor);
extern void *atheme_object_ref(void *);
extern void *atheme_object_sink_ref(void *);
extern void atheme_object_unref(void *);
extern void atheme_object_dispose(void *);

extern struct metadata *metadata_add(void *target, const char *name, const char *value);
extern void metadata_delete(void *target, const char *name);
extern struct metadata *metadata_find(void *target, const char *name);
extern void metadata_delete_all(void *target);

extern void *privatedata_get(void *target, const char *key);
extern void privatedata_set(void *target, const char *key, void *data);

#ifdef OBJECT_DEBUG
extern mowgli_list_t object_list;
#endif

#define atheme_object(x) ((struct atheme_object *) x)

#endif
