/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_dictionary.c: Dictionary-based information storage.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007 Jilles Tjoelker <jilles -at- stack.nl>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

struct mowgli_dictionary_
{
	int resolution;
	mowgli_list_t *hashv;		/* dynamically allocated by dictionary_create() */
	int (*compare_cb)(const char *a, const char *b);
	mowgli_node_t node;
};

/*
 * mowgli_dictionary_create(const char *name, int resolution,
 *                   int (*compare_cb)(const char *a, const char *b)
 *
 * Dictionary object factory.
 *
 * Inputs:
 *     - name of dictionary tree
 *     - resolution of dictionary tree
 *     - function to use for comparing two entries in the dtree
 *
 * Outputs:
 *     - on success, a new dictionary object.
 *
 * Side Effects:
 *     - if services runs out of memory and cannot allocate the object,
 *       the program will abort.
 */
mowgli_dictionary_t *mowgli_dictionary_create(int resolution, int (*compare_cb)(const char *a, const char *b))
{
	mowgli_dictionary_t *dtree = (mowgli_dictionary_t *) mowgli_alloc(sizeof(mowgli_dictionary_t));

	dtree->resolution = resolution;
	dtree->hashv      = (mowgli_list_t *) mowgli_alloc_array(sizeof(mowgli_list_t), resolution);
	dtree->compare_cb = compare_cb;

	return dtree;
}

/*
 * mowgli_dictionary_destroy(mowgli_dictionary_t *dtree,
 *     void (*destroy_cb)(dictionary_elem_t *delem, void *privdata),
 *     void *privdata);
 *
 * Recursively destroys all nodes in a dictionary tree.
 *
 * Inputs:
 *     - dictionary tree object
 *     - optional iteration callback
 *     - optional opaque/private data to pass to callback
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a dtree and optionally it's children are destroyed.
 *
 * Notes:
 *     - if this is called without a callback, the objects bound to the
 *       DTree will not be destroyed.
 */
void mowgli_dictionary_destroy(mowgli_dictionary_t *dtree,
	void (*destroy_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
	void *privdata)
{
	mowgli_node_t *n, *tn;
	int i;

	return_if_fail(dtree != NULL);

	for (i = 0; i < dtree->resolution; i++)
	{
		MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->hashv[i].head)
		{
			/* delem_t is a subclass of node_t. */
			mowgli_dictionary_elem_t *delem = (mowgli_dictionary_elem_t *) n;

			if (destroy_cb != NULL)
				(*destroy_cb)(delem, privdata);

			mowgli_node_delete(&delem->node, &dtree->hashv[i]);

			mowgli_free(delem);
		}
	}

	mowgli_free(dtree->hashv);
	mowgli_free(dtree);
}

/*
 * mowgli_dictionary_foreach(mowgli_dictionary_t *dtree,
 *     void (*destroy_cb)(dictionary_elem_t *delem, void *privdata),
 *     void *privdata);
 *
 * Iterates over all entries in a DTree.
 *
 * Inputs:
 *     - dictionary tree object
 *     - optional iteration callback
 *     - optional opaque/private data to pass to callback
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a dtree is iterated
 */
void mowgli_dictionary_foreach(mowgli_dictionary_t *dtree,
	int (*foreach_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
	void *privdata)
{
	mowgli_node_t *n, *tn;
	int i, ret = 0;

	return_if_fail(dtree != NULL);

	for (i = 0; i < dtree->resolution && ret == 0; i++)
	{
		MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->hashv[i].head)
		{
			/* delem_t is a subclass of node_t. */
			mowgli_dictionary_elem_t *delem = (mowgli_dictionary_elem_t *) n;

			if (foreach_cb != NULL)
				ret = (*foreach_cb)(delem, privdata);
		}
	}
}

/*
 * mowgli_dictionary_search(mowgli_dictionary_t *dtree,
 *     void (*destroy_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
 *     void *privdata);
 *
 * Searches all entries in a DTree using a custom callback.
 *
 * Inputs:
 *     - dictionary tree object
 *     - optional iteration callback
 *     - optional opaque/private data to pass to callback
 *
 * Outputs:
 *     - on success, the requested object
 *     - on failure, NULL.
 *
 * Side Effects:
 *     - a dtree is iterated until the requested conditions are met
 */
void *mowgli_dictionary_search(mowgli_dictionary_t *dtree,
	void *(*foreach_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
	void *privdata)
{
	mowgli_node_t *n, *tn;
	int i;
	void *ret = NULL;

	return_if_fail(dtree != NULL);

	for (i = 0; i < dtree->resolution && ret == NULL; i++)
	{
		MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->hashv[i].head)
		{
			/* delem_t is a subclass of node_t. */
			mowgli_dictionary_elem_t *delem = (mowgli_dictionary_elem_t *) n;

			if (foreach_cb != NULL)
				ret = (*foreach_cb)(delem, privdata);
		}
	}

	return ret;
}

/*
 * mowgli_dictionary_foreach_start(mowgli_dictionary_t *dtree,
 *     mowgli_dictionary_iteration_state_t *state);
 *
 * Initializes a static DTree iterator.
 *
 * Inputs:
 *     - dictionary tree object
 *     - static DTree iterator
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the static iterator, &state, is initialized.
 */
void mowgli_dictionary_foreach_start(mowgli_dictionary_t *dtree,
	mowgli_dictionary_iteration_state_t *state)
{
	return_if_fail(dtree != NULL);
	return_if_fail(state != NULL);

	state->bucket = 0;
	state->cur = NULL;
	state->next = NULL;

	/* find first item */
	while (state->bucket < dtree->resolution)
	{
		state->cur = (mowgli_dictionary_elem_t *)dtree->hashv[state->bucket].head;
		if (state->cur != NULL)
			break;
		state->bucket++;
	}

	if (state->cur == NULL)
		return;

	/* make state->cur point to first item and state->next point to
	 * second item */
	state->next = state->cur;
	mowgli_dictionary_foreach_next(dtree, state);
}

/*
 * mowgli_dictionary_foreach_cur(mowgli_dictionary_t *dtree,
 *     mowgli_dictionary_iteration_state_t *state);
 *
 * Returns the data from the current node being iterated by the
 * static iterator.
 *
 * Inputs:
 *     - dictionary tree object
 *     - static DTree iterator
 *
 * Outputs:
 *     - reference to data in the current dtree node being iterated
 *
 * Side Effects:
 *     - none
 */
void *mowgli_dictionary_foreach_cur(mowgli_dictionary_t *dtree,
	mowgli_dictionary_iteration_state_t *state)
{
	return_val_if_fail(dtree != NULL, NULL);
	return_val_if_fail(state != NULL, NULL);

	return state->cur != NULL ? state->cur->node.data : NULL;
}

/*
 * mowgli_dictionary_foreach_next(mowgli_dictionary_t *dtree,
 *     mowgli_dictionary_iteration_state_t *state);
 *
 * Advances a static DTree iterator.
 *
 * Inputs:
 *     - dictionary tree object
 *     - static DTree iterator
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the static iterator, &state, is advanced to a new DTree node.
 */
void mowgli_dictionary_foreach_next(mowgli_dictionary_t *dtree,
	mowgli_dictionary_iteration_state_t *state)
{
	return_if_fail(dtree != NULL);
	return_if_fail(state != NULL);

	if (state->cur == NULL)
	{
		mowgli_log("mowgli_dictionary_foreach_next(): called again after iteration finished on dtree<%p>", dtree);
		return;
	}
	state->cur = state->next;
	if (state->next == NULL)
		return;
	state->next = (mowgli_dictionary_elem_t *)state->next->node.next;
	if (state->next != NULL)
		return;
	while (++state->bucket < dtree->resolution)
	{
		state->next = (mowgli_dictionary_elem_t *)dtree->hashv[state->bucket].head;
		if (state->next != NULL)
			return;
	}
}

/*
 * mowgli_dictionary_find(mowgli_dictionary_t *dtree, const char *key)
 *
 * Looks up a DTree node by name.
 *
 * Inputs:
 *     - dictionary tree object
 *     - name of node to lookup
 *
 * Outputs:
 *     - on success, the dtree node requested
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
mowgli_dictionary_elem_t *mowgli_dictionary_find(mowgli_dictionary_t *dtree, const char *key)
{
	mowgli_node_t *n;
	int i;

	if (dtree == NULL || key == NULL)
		return NULL;

	i = mowgli_fnv_hash_string(key) % dtree->resolution;

	MOWGLI_LIST_FOREACH(n, dtree->hashv[i].head)
	{
		/* delem_t is a subclass of node_t. */
		mowgli_dictionary_elem_t *delem = (mowgli_dictionary_elem_t *) n;

		if (!dtree->compare_cb(key, delem->key))
			return delem;
	}

	return NULL;
}

/*
 * mowgli_dictionary_add(mowgli_dictionary_t *dtree, const char *key, void *data)
 *
 * Creates a new DTree node and binds data to it.
 *
 * Inputs:
 *     - dictionary tree object
 *     - name for new DTree node
 *     - data to bind to the new DTree node
 *
 * Outputs:
 *     - on success, a new DTree node
 *     - on failure, NULL
 *
 * Side Effects:
 *     - data is inserted into the DTree.
 */
mowgli_dictionary_elem_t *mowgli_dictionary_add(mowgli_dictionary_t *dtree, const char *key, void *data)
{
	mowgli_dictionary_elem_t *delem;
	int i;

	return_val_if_fail(dtree != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(data != NULL, NULL);
	return_val_if_fail(mowgli_dictionary_find(dtree, key) == NULL, NULL);

	i = mowgli_fnv_hash_string(key) % dtree->resolution;
	delem = (mowgli_dictionary_elem_t *) mowgli_alloc(sizeof(mowgli_dictionary_elem_t));

	delem->key = strdup(key);
	mowgli_node_add(data, &delem->node, &dtree->hashv[i]);

	return delem;
}

/*
 * mowgli_dictionary_delete(mowgli_dictionary_t *dtree, const char *key)
 *
 * Deletes data from a dictionary tree.
 *
 * Inputs:
 *     - dictionary tree object
 *     - name of DTree node to delete
 *
 * Outputs:
 *     - on success, the remaining data that needs to be mowgli_freed
 *     - on failure, NULL
 *
 * Side Effects:
 *     - data is removed from the DTree.
 *
 * Notes:
 *     - the returned data needs to be mowgli_freed/released manually!
 */
void *mowgli_dictionary_delete(mowgli_dictionary_t *dtree, const char *key)
{
	mowgli_dictionary_elem_t *delem = mowgli_dictionary_find(dtree, key);
	void *data;
	int i;

	if (delem == NULL)
	{
		mowgli_log("mowgli_dictionary_delete(): entry '%s' does not exist in dtree<%p>!",
			key, dtree);
		return NULL;
	}

	i = mowgli_fnv_hash_string(key) % dtree->resolution;

	data = delem->node.data;

	mowgli_node_delete(&delem->node, &dtree->hashv[i]);

	mowgli_free(delem);	

	return data;
}

/*
 * mowgli_dictionary_retrieve(mowgli_dictionary_t *dtree, const char *key)
 *
 * Retrieves data from a dictionary.
 *
 * Inputs:
 *     - dictionary tree object
 *     - name of node to lookup
 *
 * Outputs:
 *     - on success, the data bound to the DTree node.
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
void *mowgli_dictionary_retrieve(mowgli_dictionary_t *dtree, const char *key)
{
	mowgli_dictionary_elem_t *delem = mowgli_dictionary_find(dtree, key);

	if (delem != NULL)
		return delem->node.data;

	return NULL;
}
