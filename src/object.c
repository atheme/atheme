/*
 * atheme-services: A collection of minimalist IRC services
 * object.c: Object management.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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

#include "atheme.h"

/*
 * object_init
 *
 * Populates the object manager part of an object.
 *
 * Inputs:
 *      - pointer to object manager area
 *      - (optional) name of object
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

	if (name != NULL)
		obj->name = sstrdup(name);

	obj->destructor = des;
#ifdef USE_OBJECT_REF
	obj->refcount = 1;
#endif
}

#ifdef USE_OBJECT_REF
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
	return_val_if_fail(object != NULL, NULL);

	object(object)->refcount++;

	return object;
}
#endif

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
	object_t *obj;

	return_if_fail(object != NULL);
	obj = object(object);

#ifdef USE_OBJECT_REF
	obj->refcount--;

	if (obj->refcount <= 0)
#endif
	{
		if (obj->name != NULL)
			free(obj->name);

		if (obj->destructor != NULL)
			obj->destructor(obj);
		else
			free(obj);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
