/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 * $Id: object.c 7487 2007-01-14 08:34:12Z nenolod $
 */

#include "atheme.h"

/*
 * object_init
 *
 * Populates the object manager part of an object.
 *
 * Inputs:
 *      - pointer to object manager area
 *      - name of object
 *      - (optional) custom destructor
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - none
 */
void object_init(object_t *obj, const char *name, destructor_t des)
{
	return_if_fail(obj != NULL);
	return_if_fail(name != NULL);

	obj->name = sstrdup(name);
	obj->destructor = des;
	obj->refcount = 1;
}

/*
 * object_ref
 *
 * Increment the reference counter on an object.
 *
 * Inputs:
 *      - the object to refcount
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - none
 */
void * object_ref(void *object)
{
	return_if_fail(object != NULL);

	object(object)->refcount++;

	return object;
}

/*
 * object_unref
 *
 * Decrement the reference counter on an object.
 *
 * Inputs:
 *      - the object to refcount
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - if the refcount is 0, the object is destroyed.
 */
void object_unref(void *object)
{
	return_if_fail(object != NULL);
	object_t *obj = object(object);

	obj->refcount--;

	if (obj->refcount <= 0)
	{
		free(obj->name);

		if (obj->destructor != NULL)
			obj->destructor(obj);
		else
			free(obj);
	}
}

