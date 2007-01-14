/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- atheme.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Object management.
 *
 * $Id: object.c 7481 2007-01-14 08:19:09Z nenolod $
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
void object_init(object_t *obj, char *name, destructor_t des)
{
	return_if_fail(obj != NULL);
	return_if_fail(name != NULL);

	obj->name = name;
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
void object_ref(object_t *obj)
{
	return_if_fail(obj != NULL);

	obj->refcount++;
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
void object_unref(object_t *obj)
{
	return_if_fail(obj != NULL);

	obj->refcount--;

	if (obj->refcount <= 0)
	{
		if (obj->destructor != NULL)
			obj->destructor(obj);
		else
			free(obj);
	}
}

