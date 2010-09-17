/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_dictionary.c: Dictionary-based information storage.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007 Jilles Tjoelker <jilles -at- stack.nl>
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

static mowgli_heap_t *elem_heap = NULL;

struct mowgli_dictionary_
{
	mowgli_dictionary_comparator_func_t compare_cb;
	mowgli_dictionary_elem_t *root, *head, *tail;
	unsigned int count;
	char *id;
	mowgli_boolean_t dirty;
};

/*
 * mowgli_dictionary_create(mowgli_dictionary_comparator_func_t compare_cb)
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
mowgli_dictionary_t *mowgli_dictionary_create(mowgli_dictionary_comparator_func_t compare_cb)
{
	mowgli_dictionary_t *dtree = (mowgli_dictionary_t *) mowgli_alloc(sizeof(mowgli_dictionary_t));

	dtree->compare_cb = compare_cb;

	if (!elem_heap)
		elem_heap = mowgli_heap_create(sizeof(mowgli_dictionary_elem_t), 1024, BH_NOW);

	mowgli_log("mowgli_dictionary is deprecated and pending removal in Mowgli 1.0 series.");
	mowgli_log("Consider replacing this code with mowgli_patricia.");

	return dtree;
}

/*
 * mowgli_dictionary_create_named(const char *name, 
 *     mowgli_dictionary_comparator_func_t compare_cb)
 *
 * Dictionary object factory.
 *
 * Inputs:
 *     - dictionary name
 *     - function to use for comparing two entries in the dtree
 *
 * Outputs:
 *     - on success, a new dictionary object.
 *
 * Side Effects:
 *     - if services runs out of memory and cannot allocate the object,
 *       the program will abort.
 */
mowgli_dictionary_t *mowgli_dictionary_create_named(const char *name,
	mowgli_dictionary_comparator_func_t compare_cb)
{
	mowgli_dictionary_t *dtree = (mowgli_dictionary_t *) mowgli_alloc(sizeof(mowgli_dictionary_t));

	dtree->compare_cb = compare_cb;
	dtree->id = strdup(name);

	if (!elem_heap)
		elem_heap = mowgli_heap_create(sizeof(mowgli_dictionary_elem_t), 1024, BH_NOW);

	mowgli_log("mowgli_dictionary is deprecated and pending removal in Mowgli 1.0 series.");
	mowgli_log("Consider replacing this code with mowgli_patricia.");

	return dtree;
}

/*
 * mowgli_dictionary_set_comparator_func(mowgli_dictionary_t *dict,
 *     mowgli_dictionary_comparator_func_t compare_cb)
 *
 * Resets the comparator function used by the dictionary code for
 * updating the DTree structure.
 *
 * Inputs:
 *     - dictionary object
 *     - new comparator function (passed as functor)
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the dictionary comparator function is reset.
 */
void mowgli_dictionary_set_comparator_func(mowgli_dictionary_t *dict,
	mowgli_dictionary_comparator_func_t compare_cb)
{
	return_if_fail(dict != NULL);
	return_if_fail(compare_cb != NULL);

	dict->compare_cb = compare_cb;
}

/*
 * mowgli_dictionary_get_comparator_func(mowgli_dictionary_t *dict)
 *
 * Returns the current comparator function used by the dictionary.
 *
 * Inputs:
 *     - dictionary object
 *
 * Outputs:
 *     - comparator function (returned as functor)
 *
 * Side Effects:
 *     - none
 */
mowgli_dictionary_comparator_func_t
mowgli_dictionary_get_comparator_func(mowgli_dictionary_t *dict)
{
	return_val_if_fail(dict != NULL, NULL);

	return dict->compare_cb;
}

/*
 * mowgli_dictionary_get_linear_index(mowgli_dictionary_t *dict,
 *     const char *key)
 *
 * Gets a linear index number for key.
 *
 * Inputs:
 *     - dictionary tree object
 *     - pointer to data
 *
 * Outputs:
 *     - position, from zero.
 *
 * Side Effects:
 *     - rebuilds the linear index if the tree is marked as dirty.
 */
int
mowgli_dictionary_get_linear_index(mowgli_dictionary_t *dict, const char *key)
{
	mowgli_dictionary_elem_t *elem;

	return_val_if_fail(dict != NULL, 0);
	return_val_if_fail(key != NULL, 0);

	elem = mowgli_dictionary_find(dict, key);
	if (elem == NULL)
		return -1;

	if (!dict->dirty)
		return elem->position;
	else
	{
		mowgli_dictionary_elem_t *delem;
		int i;

		for (delem = dict->head, i = 0; delem != NULL; delem = delem->next, i++)
			delem->position = i;

		dict->dirty = FALSE;
	}

	return elem->position;
}

/*
 * mowgli_dictionary_retune(mowgli_dictionary_t *dict, const char *key)
 *
 * Retunes the tree, self-optimizing for the element which belongs to key.
 *
 * Tuning the tree structure is a very complex operation. Unlike
 * 2-3-4 trees and BTree/BTree+ structures, this structure is a
 * constantly evolving algorithm.
 *
 * Instead of maintaining a balanced tree, we constantly adapt the
 * tree by nominating a new root nearby the most recently looked up
 * or added data. We are constantly retuning ourselves instead of
 * doing massive O(n) rebalance operations as seen in BTrees,
 * and the level of data stored in a tree is dynamic, instead of being
 * held to a restricted design like other trees.
 *
 * Moreover, we are different than a radix/patricia tree, because we
 * don't statically allocate positions. Radix trees have the advantage
 * of not requiring tuning or balancing operations while having the
 * disadvantage of requiring a large amount of memory to store
 * large trees. Our efficiency as far as speed goes is not as
 * fast as a radix tree; but is close to it.
 *
 * The retuning algorithm uses the comparison callback that is
 * passed in the initialization of the tree container. If the
 * comparator returns a value which is less than zero, we push the
 * losing node out of the way, causing it to later be reparented
 * with another node. The winning child of this comparison is always
 * the right-most node.
 *
 * Once we have reached the key which has been targeted, or have reached
 * a deadend, we nominate the nearest node as the new root of the tree.
 * If an exact match has been found, the new root becomes the node which
 * represents key.
 *
 * This results in a tree which can self-optimize for both critical
 * conditions: nodes which are distant and similar and trees which
 * have ordered lookups.
 *
 * Inputs:
 *     - node to begin search from
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a new root node is nominated.
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
 * When we add new nodes to the tree, it becomes the
 * next nominated root. This is perhaps not a wise
 * optimization because of automatic retuning, but
 * it keeps the code simple.
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

	dict->dirty = TRUE;

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

			mowgli_heap_free(elem_heap, delem);
		}
	}
}

/*
 * mowgli_dictionary_unlink_root(mowgli_dictionary_t *dict)
 *
 * Unlinks the root dictionary tree element from the dictionary.
 *
 * Inputs:
 *     - dictionary tree
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the root node is unlinked from the dictionary tree
 */
void
mowgli_dictionary_unlink_root(mowgli_dictionary_t *dict)
{
	mowgli_dictionary_elem_t *delem, *nextnode, *parentofnext;

	dict->dirty = TRUE;

	delem = dict->root;
	if (delem == NULL)
		return;

	if (dict->root->left == NULL)
		dict->root = dict->root->right;
	else if (dict->root->right == NULL)
		dict->root = dict->root->left;
	else
	{
		/* Make the node with the next highest key the new root.
		 * This node has a NULL left pointer. */
		nextnode = delem->next;
		soft_assert(nextnode->left == NULL);
		if (nextnode == delem->right)
		{
			dict->root = nextnode;
			dict->root->left = delem->left;
		}
		else
		{
			parentofnext = delem->right;
			while (parentofnext->left != NULL && parentofnext->left != nextnode)
				parentofnext = parentofnext->left;
			soft_assert(parentofnext->left == nextnode);
			parentofnext->left = nextnode->right;
			dict->root = nextnode;
			dict->root->left = delem->left;
			dict->root->right = delem->right;
		}
	}

	/* linked list */
	if (delem->prev != NULL)
		delem->prev->next = delem->next;

	if (dict->head == delem)
		dict->head = delem->next;

	if (delem->next)
		delem->next->prev = delem->prev;

	if (dict->tail == delem)
		dict->tail = delem->prev;

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

	return_if_fail(dtree != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->head)
	{
		if (destroy_cb != NULL)
			(*destroy_cb)(n, privdata);

		mowgli_free(n->key);
		mowgli_heap_free(elem_heap, n);
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
	void *ret = NULL;

	return_val_if_fail(dtree != NULL, NULL);

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

	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);
	return_val_if_fail(data != NULL, NULL);
	return_val_if_fail(mowgli_dictionary_find(dict, key) == NULL, NULL);

	delem = mowgli_heap_alloc(elem_heap);
	delem->key = strdup(key);
	delem->data = data;

	if (delem->key == NULL)
	{
		mowgli_log("major WTF: delem->key is NULL, not adding node.", key);
		mowgli_heap_free(elem_heap, delem);
		return NULL;
	}

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

	if (delem == NULL)
		return NULL;

	data = delem->data;

	mowgli_free(delem->key);
	mowgli_dictionary_unlink_root(dtree);
	mowgli_heap_free(elem_heap, delem);	

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

/* returns the sum of the depths of the subtree rooted in delem at depth depth */
static int
stats_recurse(mowgli_dictionary_elem_t *delem, int depth, int *pmaxdepth)
{
	int result;

	if (depth > *pmaxdepth)
		*pmaxdepth = depth;
	result = depth;
	if (delem->left)
		result += stats_recurse(delem->left, depth + 1, pmaxdepth);
	if (delem->right)
		result += stats_recurse(delem->right, depth + 1, pmaxdepth);
	return result;
}

/*
 * mowgli_dictionary_stats(mowgli_dictionary_t *dict, void (*cb)(const char *line, void *privdata), void *privdata)
 *
 * Returns the size of a dictionary.
 *
 * Inputs:
 *     - dictionary tree object
 *     - callback
 *     - data for callback
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - callback called with stats text
 */
void mowgli_dictionary_stats(mowgli_dictionary_t *dict, void (*cb)(const char *line, void *privdata), void *privdata)
{
	char str[256];
	int sum, maxdepth;

	return_if_fail(dict != NULL);

	if (dict->id != NULL)
		snprintf(str, sizeof str, "Dictionary stats for %s (%d)",
				dict->id, dict->count);
	else
		snprintf(str, sizeof str, "Dictionary stats for <%p> (%d)",
				dict, dict->count);
	cb(str, privdata);
	maxdepth = 0;
	if (dict->root != NULL)
	{
		sum = stats_recurse(dict->root, 0, &maxdepth);
		snprintf(str, sizeof str, "Depth sum %d Avg depth %d Max depth %d", sum, sum / dict->count, maxdepth);
	}
	else
		snprintf(str, sizeof str, "Depth sum 0 Avg depth 0 Max depth 0");
	cb(str, privdata);
	return;
}
