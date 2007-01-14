/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 * $Id: object.h 7481 2007-01-14 08:19:09Z nenolod $
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

#endif
