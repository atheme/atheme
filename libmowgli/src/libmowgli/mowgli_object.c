/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object.c: Object management.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "mowgli.h"

/*
 * mowgli_object_init
 *
 * Populates the object manager part of an object.
 *
 * Inputs:
 *      - pointer to object manager area
 *      - (optional) name of object
 *      - (optional) class of object
 *      - (optional) custom destructor
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - none
 */
void mowgli_object_init(mowgli_object_t *obj, const char *name, mowgli_object_class_t *klass, mowgli_destructor_t des)
{
	return_if_fail(obj != NULL);

	if (name != NULL)
		obj->name = strdup(name);

	if (klass != NULL)
		obj->klass = klass;
	else
	{
		mowgli_object_class_t *tmp = mowgli_alloc(sizeof(mowgli_object_class_t));
		mowgli_object_class_init(tmp, name, des, TRUE);
		obj->klass = tmp;
	}

	obj->refcount = 1;

	obj->message_handlers.head = NULL;
	obj->message_handlers.tail = NULL;
	obj->message_handlers.count = 0;

	obj->metadata.head = NULL;
	obj->metadata.tail = NULL;
	obj->metadata.count = 0;

	mowgli_object_message_broadcast(obj, "create");
}

/*
 * mowgli_object_init_from_class
 *
 * Populates the object manager part of an object from an object class.
 *
 * Inputs:
 *      - pointer to object manager area
 *      - class of object
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - none
 */
void
mowgli_object_init_from_class(mowgli_object_t *obj, const char *name,
	mowgli_object_class_t *klass)
{
	return_if_fail(obj != NULL);
	return_if_fail(klass != NULL);

	mowgli_object_init(obj, name, klass, NULL);
}

/*
 * mowgli_object_ref
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
void * mowgli_object_ref(void *object)
{
	return_val_if_fail(object != NULL, NULL);

	mowgli_object(object)->refcount++;

	return object;
}

/*
 * mowgli_object_unref
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
void mowgli_object_unref(void *object)
{
	mowgli_object_t *obj = mowgli_object(object);

	return_if_fail(object != NULL);

	obj->refcount--;

	if (obj->refcount <= 0)
	{
		mowgli_object_message_broadcast(obj, "destroy");

		if (obj->name != NULL)
			free(obj->name);

		if (obj->klass != NULL)
		{
			mowgli_destructor_t destructor = obj->klass->destructor;

			if (obj->klass->dynamic == TRUE)
				mowgli_object_class_destroy(obj->klass);

			if (destructor != NULL)
				destructor(obj);
			else
				free(obj);
		}
		else
			mowgli_throw_exception(mowgli.object.invalid_object_class_exception);
	}
}
