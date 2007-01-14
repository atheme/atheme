/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 * $Id: object.h 7483 2007-01-14 08:22:28Z nenolod $
 */

#ifndef __ATHEME_OBJECT_H__
#define __ATHEME_OBJECT_H__

typedef void (*destructor_t)(void *);

typedef struct {
	int refcount;
	destructor_t destructor;
} object_t;

E void object_init(object_t *, char *name, destructor_t destructor);
E void object_ref(object_t *);
E void object_unref(object_t *);

#define object(x) ((object_t *) x)

#endif
