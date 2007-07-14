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
	int (*compare_cb)(const char *a, const char *b);
	mowgli_dictionary_elem_t *root, *head, *tail;
	unsigned int count;
};

/*
 * mowgli_dictionary_create(const char *name, int resolution,
 *                   int (*compare_cb)(const char *a, const char *b)
 *
 * Dictionary object factory.
 *
 * Inputs:
 *     - function to use for comparing two entries in the dtree
 *
 * Outputs:
 *     - on success, a new dictionary object.
 *
 * Side Effects:
 *     - if services runs out of memory and cannot allocate the object,
 *       the program will abort.
 */
mowgli_dictionary_t *mowgli_dictionary_create(int (*compare_cb)(const char *a, const char *b))
{
	mowgli_dictionary_t *dtree = (mowgli_dictionary_t *) mowgli_alloc(sizeof(mowgli_dictionary_t));

	dtree->compare_cb = compare_cb;

	return dtree;
}

/*
 * mowgli_dictionary_retune(mowgli_dictionary_t *dicy, const char *key)
 *
 * Rebalances the tree around the element which belongs to key.
 *
 * Inputs:
 *     - node to begin search from
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - the tree is rebalanced.
 */
void
mowgli_dictionary_retune(mowgli_dictionary_t *dict, const char *key)
{
	mowgli_dictionary_elem_t n, *tn, *left, *right, *node;
	int ret;

	return_if_fail(dict != NULL);

	if (dict->root == NULL)
		return;

	/*
	 * we initialize n with known values, since it's on stack
	 * memory. otherwise the dict would become corrupted.
	 *
 	 * n is used for temporary storage while the tree is retuned.
	 *    -nenolod
	 */
	n.left = n.right = NULL;
	left = right = &n;

	/* this for(;;) loop is the main workhorse of the rebalancing */
	for (node = dict->root; ; )
	{
		if ((ret = dict->compare_cb(key, node->key)) == 0)
			break;

		if (ret < 0)
		{
			if (node->left == NULL)
				break;

			if ((ret = dict->compare_cb(key, node->left->key)) < 0)
			{
				tn = node->left;
				node->left = tn->right;
				tn->right = node;
				node = tn;

				if (node->left == NULL)
					break;
			}

			right->left = node;
			right = node;
			node = node->left;
		}
		else
		{
			if (node->right == NULL)
				break;

			if ((ret = dict->compare_cb(key, node->right->key)) > 0)
			{
				tn = node->right;
				node->right = tn->left;
				tn->left = node;
				node = tn;

				if (node->right == NULL)
					break;
			}

			left->right = node;
			left = node;
			node = node->right;
		}
	}

	left->right = node->left;
	right->left = node->right;

	node->left = n.right;
	node->right = n.left;

	dict->root = node;
}

/*
 * mowgli_dictionary_link(mowgli_dictionary_t *dict,
 *     mowgli_dictionary_elem_t *delem)
 *
 * Links a dictionary tree element to the dictionary.
 *
 * Inputs:
 *     - dictionary tree
 *     - dictionary tree element
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a node is linked to the dictionary tree
 */
void
mowgli_dictionary_link(mowgli_dictionary_t *dict,
	mowgli_dictionary_elem_t *delem)
{
	return_if_fail(dict != NULL);
	return_if_fail(delem != NULL);

	dict->count++;

	if (dict->root == NULL)
	{
		delem->left = delem->right = NULL;
		delem->next = delem->prev = NULL;
		dict->head = dict->tail = dict->root = delem;
	}
	else
	{
		int ret;

		mowgli_dictionary_retune(dict, delem->key);

		if ((ret = dict->compare_cb(delem->key, dict->root->key)) < 0)
		{
			delem->left = dict->root->left;
			delem->right = dict->root;
			dict->root->left = NULL;

			if (dict->root->prev)
				dict->root->prev->next = delem;
			else
				dict->head = delem;

			delem->prev = dict->root->prev;
			delem->next = dict->root;
			dict->root->prev = delem;
			dict->root = delem;
		}
		else if (ret > 0)
		{
			delem->right = dict->root->right;
			delem->left = dict->root;
			dict->root->right = NULL;

			if (dict->root->next)
				dict->root->next->prev = delem;
			else
				dict->tail = delem;

			delem->next = dict->root->next;
			delem->prev = dict->root;
			dict->root->next = delem;
			dict->root = delem;
		}
		else
		{
			dict->root->key = delem->key;
			dict->root->data = delem->data;
			dict->count--;

			mowgli_free(delem);
		}
	}
}

/*
 * mowgli_dictionary_unlink(mowgli_dictionary_t *dict,
 *     mowgli_dictionary_elem_t *delem)
 *
 * Unlinks a dictionary tree element from the dictionary.
 *
 * Inputs:
 *     - dictionary tree
 *     - dictionary tree element
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a node is unlinked from the dictionary tree
 */
void
mowgli_dictionary_unlink(mowgli_dictionary_t *dict,
	mowgli_dictionary_elem_t *delem)
{
	mowgli_dictionary_elem_t *old_root, *new_root;

	if (dict->root == NULL || delem == NULL)
		return;

	dict->root = delem;
	if (dict->compare_cb(delem->key, dict->root->key) != 0)
		return;

	if (dict->root->left == NULL)
		new_root = dict->root->right;
	else
	{
		dict->root = dict->root->left;
		mowgli_dictionary_retune(dict, delem->key);
		new_root = dict->root;
		new_root->right = dict->root->right;
	}

	/* linked list */
	if (dict->root->prev != NULL)
		dict->root->prev->next = dict->root->next;

	if (dict->head == dict->root)
		dict->head = dict->root->next;

	if (dict->root->next)
		dict->root->next->prev = dict->root->prev;

	if (dict->tail == dict->root)
		dict->tail = dict->root->prev;

	old_root = dict->root;
	dict->root = new_root;
	dict->count--;
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
	mowgli_dictionary_elem_t *n, *tn;
	int i;

	return_if_fail(dtree != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->head)
	{
		if (destroy_cb != NULL)
			(*destroy_cb)(n, privdata);

		mowgli_dictionary_unlink(dtree, n);
		mowgli_free(n);
	}

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
	mowgli_dictionary_elem_t *n, *tn;

	return_if_fail(dtree != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->head)
	{
		/* delem_t is a subclass of node_t. */
		mowgli_dictionary_elem_t *delem = (mowgli_dictionary_elem_t *) n;

		if (foreach_cb != NULL)
			(*foreach_cb)(delem, privdata);
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
	mowgli_dictionary_elem_t *n, *tn;
	int i;
	void *ret = NULL;

	return_if_fail(dtree != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->head)
	{
		/* delem_t is a subclass of node_t. */
		mowgli_dictionary_elem_t *delem = (mowgli_dictionary_elem_t *) n;

		if (foreach_cb != NULL)
			ret = (*foreach_cb)(delem, privdata);

		if (ret)
			break;
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

	state->cur = NULL;
	state->next = NULL;

	/* find first item */
	state->cur = dtree->head;

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

	return state->cur != NULL ? state->cur->data : NULL;
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

	state->next = state->next->next;
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
mowgli_dictionary_elem_t *mowgli_dictionary_find(mowgli_dictionary_t *dict, const char *key)
{
	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	/* retune for key, key will be the tree's root if it's available */
	mowgli_dictionary_retune(dict, key);

	if (dict->root && !dict->compare_cb(key, dict->root->key))
		return dict->root;

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
mowgli_dictionary_elem_t *mowgli_dictionary_add(mowgli_dictionary_t *dict, const char *key, void *data)
{
	mowgli_dictionary_elem_t *delem;
	int i;

	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(data != NULL, NULL);
	return_val_if_fail(mowgli_dictionary_find(dict, key) == NULL, NULL);

	delem = (mowgli_dictionary_elem_t *) mowgli_alloc(sizeof(mowgli_dictionary_elem_t));
	delem->key = strdup(key);
	delem->data = data;

	mowgli_dictionary_link(dict, delem);

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
		return NULL;

	data = delem->data;

	mowgli_dictionary_unlink(dtree, delem);
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
		return delem->data;

	return NULL;
}

/*
 * mowgli_dictionary_size(mowgli_dictionary_t *dict)
 *
 * Returns the size of a dictionary.
 *
 * Inputs:
 *     - dictionary tree object
 *
 * Outputs:
 *     - size of dictionary
 *
 * Side Effects:
 *     - none
 */
unsigned int mowgli_dictionary_size(mowgli_dictionary_t *dict)
{
	return_val_if_fail(dict != NULL, 0);

	return dict->count;
}
