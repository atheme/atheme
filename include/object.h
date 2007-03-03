/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 * $Id: object.h 7771 2007-03-03 12:46:36Z pippijn $
 */

#ifndef __ATHEME_OBJECT_H__
#define __ATHEME_OBJECT_H__

typedef void (*destructor_t)(void *);

typedef struct {
	char *name;
	int refcount;
	destructor_t destructor;
} object_t;

E void object_init(object_t *, const char *name, destructor_t destructor);
E void *object_ref(void *);
E void object_unref(void *);

#define object(x) ((object_t *) x)

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
