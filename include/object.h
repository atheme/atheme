/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 */

#ifndef ATHEME_OBJECT_H
#define ATHEME_OBJECT_H

struct metadata_ {
	stringref name;
	char *value;
};

typedef struct metadata_ metadata_t;

typedef void (*destructor_t)(void *);

typedef struct {
	int refcount;
	destructor_t destructor;
	mowgli_patricia_t *metadata;
	mowgli_patricia_t *privatedata;
#ifdef OBJECT_DEBUG
	mowgli_node_t dnode;
#endif
} object_t;

E void init_metadata(void);

E void object_init(object_t *, const char *name, destructor_t destructor);
E void *object_ref(void *);
E void *object_sink_ref(void *);
E void object_unref(void *);
E void object_dispose(void *);

E metadata_t *metadata_add(void *target, const char *name, const char *value);
E void metadata_delete(void *target, const char *name);
E metadata_t *metadata_find(void *target, const char *name);
E void metadata_delete_all(void *target);

E void *privatedata_get(void *target, const char *key);
E void privatedata_set(void *target, const char *key, void *data);

#ifdef OBJECT_DEBUG
E mowgli_list_t object_list;
#endif

#define object(x) ((object_t *) x)

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
