/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 */

#ifndef __ATHEME_OBJECT_H__
#define __ATHEME_OBJECT_H__

struct metadata_ {
	char *name;
	char *value;
	bool private;
	mowgli_node_t node;
};

typedef struct metadata_ metadata_t;

typedef struct {
	myuser_t *mu;
	mowgli_list_t taglist;
} metadata_subscription_t;

typedef void (*destructor_t)(void *);

typedef struct {
	char *name;
#ifdef USE_OBJECT_REF
	int refcount;
#endif
	destructor_t destructor;
	mowgli_list_t metadata;
	mowgli_patricia_t *privatedata;
} object_t;

E void init_metadata(void);

E void object_init(object_t *, const char *name, destructor_t destructor);
#ifdef USE_OBJECT_REF
E void *object_ref(void *);
#endif
E void object_unref(void *);

E metadata_t *metadata_add(void *target, const char *name, const char *value);
E void metadata_delete(void *target, const char *name);
E metadata_t *metadata_find(void *target, const char *name);
E void metadata_delete_all(void *target);

E void *privatedata_get(void *target, const char *key);
E void privatedata_set(void *target, const char *key, void *data);

#define object(x) ((object_t *) x)

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
