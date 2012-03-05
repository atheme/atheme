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

#ifdef OBJECT_DEBUG
mowgli_list_t object_list = { NULL, NULL, 0 };
#endif

mowgli_heap_t *metadata_heap;	/* HEAP_CHANUSER */

void init_metadata(void)
{
	metadata_heap = sharedheap_get(sizeof(metadata_t));

	if (metadata_heap == NULL)
	{
		slog(LG_ERROR, "init_metadata(): block allocator failure.");
		exit(EXIT_FAILURE);
	}
}

/*
 * object_init
 *
 * Populates the object manager part of an object.
 *
 * Inputs:
 *      - pointer to object manager area
 *      - (optional) name of object
 *      - (optional) custom destructor; if this is specified it must free
 *        the metadata itself
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

	obj->destructor = des;
	obj->refcount = 1;

#ifdef OBJECT_DEBUG
	mowgli_node_add(obj, &obj->dnode, &object_list);
#endif
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
	return_val_if_fail(object != NULL, NULL);

	/*
	 * refcount == 0 really should not be possible here
	 * but in fact it happens for modules/exttarget/.  --jilles
	 *
	 * actually... refcount == 0 means the object is initially
	 * unowned, which is intentional for the exttarget code, the
	 * exttarget virtual entity object is built by a 'factory' so
	 * we want to sink the reference to make it an unowned object.
	 *   --nenolod
	 */
	return_val_if_fail(object(object)->refcount >= 0, object);

	object(object)->refcount++;
#ifdef DEBUG_OBJECT_REF
	slog(LG_DEBUG, "object_ref(%p): %d references", object, object(object)->refcount);
#endif

	return object;
}

/*
 * object_sink_ref
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
 *      - none
 */
void *object_sink_ref(void *obj)
{
	return_val_if_fail(obj != NULL, NULL);
	return_val_if_fail(object(obj)->refcount > 0, obj);
	object(obj)->refcount--;

#ifdef DEBUG_OBJECT_REF
	slog(LG_DEBUG, "object_sink_ref(%p): %d references", obj, object(obj)->refcount);
#endif

	return obj;
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
void object_unref(void *obj)
{
	return_if_fail(obj != NULL);
	return_if_fail(object(obj)->refcount >= -1);

	if (object(obj)->refcount == -1)
		return;
	else if (object(obj)->refcount > 0)
	{
		object_sink_ref(obj);
		if (object(obj)->refcount == 0)
			object_dispose(obj);
	}
}

/*
 * object_dispose
 *
 * Disposes of an object.
 *
 * Inputs:
 *      - the object to dispose
 *
 * Outputs:
 *      - none
 *
 * Side Effects:
 *      - the object is destroyed
 */
void object_dispose(void *object)
{
	object_t *obj;
	mowgli_patricia_t *privatedata, *metadata;

	return_if_fail(object != NULL);
	obj = object(object);

	/* we shouldn't be disposing an object more than once in it's
	 * lifecycle.  so do a soft assert and get out if that's the case.
	 *    --nenolod
	 */
	return_if_fail(obj->refcount != -1);

	/* set refcount to -1 to ensure that object_unref() doesn't cause a loop */
	obj->refcount = -1;

	privatedata = obj->privatedata;
	metadata = obj->metadata;

#ifdef OBJECT_DEBUG
	mowgli_node_delete(&obj->dnode, &object_list);
#endif

	if (obj->destructor != NULL)
		obj->destructor(obj);
	else
	{
		metadata_delete_all(obj);
		free(obj);
	}

	if (privatedata != NULL)
		mowgli_patricia_destroy(privatedata, NULL, NULL);

	if (metadata != NULL)
		mowgli_patricia_destroy(metadata, NULL, NULL);
}

metadata_t *metadata_add(void *target, const char *name, const char *value)
{
	object_t *obj;
	metadata_t *md;

	return_val_if_fail(name != NULL, NULL);
	return_val_if_fail(value != NULL, NULL);

	obj = object(target);

	if (obj->metadata == NULL)
		obj->metadata = mowgli_patricia_create(strcasecanon);

	if (metadata_find(target, name))
		metadata_delete(target, name);

	md = mowgli_heap_alloc(metadata_heap);

	md->name = strshare_get(name);
	md->value = sstrdup(value);

	mowgli_patricia_add(obj->metadata, md->name, md);

	return md;
}

void metadata_delete(void *target, const char *name)
{
	object_t *obj;
	metadata_t *md = metadata_find(target, name);

	if (!md)
		return;

	obj = object(target);

	if (obj->metadata == NULL)
		obj->metadata = mowgli_patricia_create(strcasecanon);

	mowgli_patricia_delete(obj->metadata, name);

	strshare_unref(md->name);
	free(md->value);

	mowgli_heap_free(metadata_heap, md);
}

metadata_t *metadata_find(void *target, const char *name)
{
	object_t *obj;

	return_val_if_fail(target != NULL, NULL);
	return_val_if_fail(name != NULL, NULL);

	obj = object(target);

	if (obj->metadata == NULL)
		obj->metadata = mowgli_patricia_create(strcasecanon);

	return mowgli_patricia_retrieve(obj->metadata, name);
}

void metadata_delete_all(void *target)
{
	object_t *obj;
	metadata_t *md;
	mowgli_patricia_iteration_state_t state;

	obj = object(target);

	if (obj->metadata == NULL)
		obj->metadata = mowgli_patricia_create(strcasecanon);

	MOWGLI_PATRICIA_FOREACH(md, &state, obj->metadata)
	{
		metadata_delete(obj, md->name);
	}
}

void *privatedata_get(void *target, const char *key)
{
	object_t *obj;

	obj = object(target);
	if (obj->privatedata == NULL)
		return NULL;

	return mowgli_patricia_retrieve(obj->privatedata, key);
}

void privatedata_set(void *target, const char *key, void *data)
{
	object_t *obj;

	obj = object(target);
	if (obj->privatedata == NULL)
		obj->privatedata = mowgli_patricia_create(noopcanon);

	mowgli_patricia_add(obj->privatedata, key, data);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

