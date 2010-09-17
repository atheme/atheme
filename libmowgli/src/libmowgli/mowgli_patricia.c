/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_patricia.c: Dictionary-based information storage.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007-2010 Jilles Tjoelker <jilles -at- stack.nl>
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

static mowgli_heap_t *leaf_heap = NULL;
static mowgli_heap_t *node_heap = NULL;

/*
 * Patricia tree.
 *
 * A radix trie that avoids one-way branching and redundant nodes.
 *
 * To find a node, the tree is traversed starting from the root. The
 * nibnum in each node indicates which nibble of the key needs to be
 * tested, and the appropriate branch is taken.
 *
 * The nibnum values are strictly increasing while going down the tree.
 *
 * -- jilles
 */

union patricia_elem;

struct mowgli_patricia_
{
	void (*canonize_cb)(char *key);
	union patricia_elem *root;
	unsigned int count;
	char *id;
};

#define POINTERS_PER_NODE 16
#define NIBBLE_VAL(key, nibnum) (((key)[(nibnum) / 2] >> ((nibnum) & 1 ? 0 : 4)) & 0xF)

struct patricia_node
{
	/* nibble to test (nibble NUM%2 of byte NUM/2) */
	int nibnum;
	/* branches of the tree */
	union patricia_elem *down[POINTERS_PER_NODE];
	union patricia_elem *parent;
	char parent_val;
};

struct patricia_leaf
{
	/* -1 to indicate this is a leaf, not a node */
	int nibnum;
	/* data associated with the key */
	void *data;
	/* key (canonized copy) */
	char *key;
	union patricia_elem *parent;
	char parent_val;
};

union patricia_elem
{
	int nibnum;
	struct patricia_node node;
	struct patricia_leaf leaf;
};

#define IS_LEAF(elem) ((elem)->nibnum == -1)

/* Preserve compatibility with the old mowgli_patricia.h */
#define STATE_CUR(state) ((state)->pspare[0])
#define STATE_NEXT(state) ((state)->pspare[1])

/*
 * first_leaf()
 *
 * Find the smallest leaf hanging off a subtree.
 *
 * Inputs:
 *     - element (may be leaf or node) heading subtree
 *
 * Outputs:
 *     - lowest leaf in subtree
 *
 * Side Effects:
 *     - none
 */
static union patricia_elem *first_leaf(union patricia_elem *delem)
{
	int val;

	while (!IS_LEAF(delem))
	{
		for (val = 0; val < POINTERS_PER_NODE; val++)
		{
			if (delem->node.down[val] != NULL)
			{
				delem = delem->node.down[val];
				break;
			}
		}
	}
	return delem;
}

/*
 * mowgli_patricia_create(void (*canonize_cb)(char *key))
 *
 * Dictionary object factory.
 *
 * Inputs:
 *     - function to use for canonizing keys (for example, use
 *       a function that makes the string upper case to create
 *       a patricia with case-insensitive matching)
 *
 * Outputs:
 *     - on success, a new patricia object.
 *
 * Side Effects:
 *     - if services runs out of memory and cannot allocate the object,
 *       the program will abort.
 */
mowgli_patricia_t *mowgli_patricia_create(void (*canonize_cb)(char *key))
{
	mowgli_patricia_t *dtree = (mowgli_patricia_t *) mowgli_alloc(sizeof(mowgli_patricia_t));

	dtree->canonize_cb = canonize_cb;

	if (!leaf_heap)
		leaf_heap = mowgli_heap_create(sizeof(struct patricia_leaf), 1024, BH_NOW);
	if (!node_heap)
		node_heap = mowgli_heap_create(sizeof(struct patricia_node), 128, BH_NOW);

	dtree->root = NULL;

	return dtree;
}

/*
 * mowgli_patricia_create_named(const char *name, 
 *     void (*canonize_cb)(char *key))
 *
 * Dictionary object factory.
 *
 * Inputs:
 *     - patricia name
 *     - function to use for canonizing keys (for example, use
 *       a function that makes the string upper case to create
 *       a patricia with case-insensitive matching)
 *
 * Outputs:
 *     - on success, a new patricia object.
 *
 * Side Effects:
 *     - if services runs out of memory and cannot allocate the object,
 *       the program will abort.
 */
mowgli_patricia_t *mowgli_patricia_create_named(const char *name,
	void (*canonize_cb)(char *key))
{
	mowgli_patricia_t *dtree = (mowgli_patricia_t *) mowgli_alloc(sizeof(mowgli_patricia_t));

	dtree->canonize_cb = canonize_cb;
	dtree->id = strdup(name);

	if (!leaf_heap)
		leaf_heap = mowgli_heap_create(sizeof(struct patricia_leaf), 1024, BH_NOW);
	if (!node_heap)
		node_heap = mowgli_heap_create(sizeof(struct patricia_node), 128, BH_NOW);

	dtree->root = NULL;

	return dtree;
}

/*
 * mowgli_patricia_destroy(mowgli_patricia_t *dtree,
 *     void (*destroy_cb)(const char *key, void *data, void *privdata),
 *     void *privdata);
 *
 * Recursively destroys all nodes in a patricia tree.
 *
 * Inputs:
 *     - patricia tree object
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
void mowgli_patricia_destroy(mowgli_patricia_t *dtree,
	void (*destroy_cb)(const char *key, void *data, void *privdata),
	void *privdata)
{
	mowgli_patricia_iteration_state_t state;
	union patricia_elem *delem;
	void *entry;

	return_if_fail(dtree != NULL);

	MOWGLI_PATRICIA_FOREACH(entry, &state, dtree)
	{
		delem = STATE_CUR(&state);
		if (destroy_cb != NULL)
			(*destroy_cb)(delem->leaf.key, delem->leaf.data,
					privdata);
		mowgli_patricia_delete(dtree, delem->leaf.key);
	}

	mowgli_free(dtree);
}

/*
 * mowgli_patricia_foreach(mowgli_patricia_t *dtree,
 *     int (*foreach_cb)(const char *key, void *data, void *privdata),
 *     void *privdata);
 *
 * Iterates over all entries in a DTree.
 *
 * Inputs:
 *     - patricia tree object
 *     - optional iteration callback
 *     - optional opaque/private data to pass to callback
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - on success, a dtree is iterated
 */
void mowgli_patricia_foreach(mowgli_patricia_t *dtree,
	int (*foreach_cb)(const char *key, void *data, void *privdata),
	void *privdata)
{
	union patricia_elem *delem, *next;
	int val;

	return_if_fail(dtree != NULL);

	delem = dtree->root;
	if (delem == NULL)
		return;
	/* Only one element in the tree */
	if (IS_LEAF(delem))
	{
		if (foreach_cb != NULL)
			(*foreach_cb)(delem->leaf.key, delem->leaf.data, privdata);
		return;
	}
	val = 0;
	do
	{
		do
			next = delem->node.down[val++];
		while (next == NULL && val < POINTERS_PER_NODE);
		if (next != NULL)
		{
			if (IS_LEAF(next))
			{
				if (foreach_cb != NULL)
					(*foreach_cb)(next->leaf.key, next->leaf.data, privdata);
			}
			else
			{
				delem = next;
				val = 0;
			}
		}
		while (val >= POINTERS_PER_NODE)
		{
			val = delem->node.parent_val;
			delem = delem->node.parent;
			if (delem == NULL)
				break;
			val++;
		}
	} while (delem != NULL);
}

/*
 * mowgli_patricia_search(mowgli_patricia_t *dtree,
 *     void *(*foreach_cb)(const char *key, void *data, void *privdata),
 *     void *privdata);
 *
 * Searches all entries in a DTree using a custom callback.
 *
 * Inputs:
 *     - patricia tree object
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
void *mowgli_patricia_search(mowgli_patricia_t *dtree,
	void *(*foreach_cb)(const char *key, void *data, void *privdata),
	void *privdata)
{
	union patricia_elem *delem, *next;
	int val;
	void *ret = NULL;

	return_val_if_fail(dtree != NULL, NULL);

	delem = dtree->root;
	if (delem == NULL)
		return NULL;
	/* Only one element in the tree */
	if (IS_LEAF(delem))
	{
		if (foreach_cb != NULL)
			return (*foreach_cb)(delem->leaf.key, delem->leaf.data, privdata);
		return NULL;
	}
	val = 0;
	for (;;)
	{
		do
			next = delem->node.down[val++];
		while (next == NULL && val < POINTERS_PER_NODE);
		if (next != NULL)
		{
			if (IS_LEAF(next))
			{
				if (foreach_cb != NULL)
					ret = (*foreach_cb)(next->leaf.key, next->leaf.data, privdata);
				if (ret != NULL)
					break;
			}
			else
			{
				delem = next;
				val = 0;
			}
		}
		while (val >= POINTERS_PER_NODE)
		{
			val = delem->node.parent_val;
			delem = delem->node.parent;
			if (delem == NULL)
				break;
			val++;
		}
	}
	return ret;
}

/*
 * mowgli_patricia_foreach_start(mowgli_patricia_t *dtree,
 *     mowgli_patricia_iteration_state_t *state);
 *
 * Initializes a static DTree iterator.
 *
 * Inputs:
 *     - patricia tree object
 *     - static DTree iterator
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the static iterator, &state, is initialized.
 */
void mowgli_patricia_foreach_start(mowgli_patricia_t *dtree,
	mowgli_patricia_iteration_state_t *state)
{
	return_if_fail(dtree != NULL);
	return_if_fail(state != NULL);

	if (dtree->root != NULL)
		STATE_NEXT(state) = first_leaf(dtree->root);
	else
		STATE_NEXT(state) = NULL;
	STATE_CUR(state) = STATE_NEXT(state);

	if (STATE_NEXT(state) == NULL)
		return;

	/* make STATE_CUR point to first item and STATE_NEXT point to
	 * second item */
	mowgli_patricia_foreach_next(dtree, state);
}

/*
 * mowgli_patricia_foreach_cur(mowgli_patricia_t *dtree,
 *     mowgli_patricia_iteration_state_t *state);
 *
 * Returns the data from the current node being iterated by the
 * static iterator.
 *
 * Inputs:
 *     - patricia tree object
 *     - static DTree iterator
 *
 * Outputs:
 *     - reference to data in the current dtree node being iterated
 *
 * Side Effects:
 *     - none
 */
void *mowgli_patricia_foreach_cur(mowgli_patricia_t *dtree,
	mowgli_patricia_iteration_state_t *state)
{
	return_val_if_fail(dtree != NULL, NULL);
	return_val_if_fail(state != NULL, NULL);

	return STATE_CUR(state) != NULL ?
		((struct patricia_leaf *)STATE_CUR(state))->data : NULL;
}

/*
 * mowgli_patricia_foreach_next(mowgli_patricia_t *dtree,
 *     mowgli_patricia_iteration_state_t *state);
 *
 * Advances a static DTree iterator.
 *
 * Inputs:
 *     - patricia tree object
 *     - static DTree iterator
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - the static iterator, &state, is advanced to a new DTree node.
 */
void mowgli_patricia_foreach_next(mowgli_patricia_t *dtree,
	mowgli_patricia_iteration_state_t *state)
{
	struct patricia_leaf *leaf;
	union patricia_elem *delem, *next;
	int val;

	return_if_fail(dtree != NULL);
	return_if_fail(state != NULL);

	if (STATE_CUR(state) == NULL)
	{
		mowgli_log("mowgli_patricia_foreach_next(): called again after iteration finished on dtree<%p>", dtree);
		return;
	}

	STATE_CUR(state) = STATE_NEXT(state);

	if (STATE_NEXT(state) == NULL)
		return;

	leaf = STATE_NEXT(state);
	delem = leaf->parent;
	val = leaf->parent_val;

	while (delem != NULL)
	{
		do
			next = delem->node.down[val++];
		while (next == NULL && val < POINTERS_PER_NODE);
		if (next != NULL)
		{
			if (IS_LEAF(next))
			{
				/* We will find the original leaf first. */
				if (&next->leaf != leaf)
				{
					if (strcmp(next->leaf.key, leaf->key) < 0)
					{
						mowgli_log("mowgli_patricia_foreach_next(): iteration went backwards (libmowgli bug) on dtree<%p>", dtree);
						STATE_NEXT(state) = NULL;
						return;
					}
					STATE_NEXT(state) = next;
					return;
				}
			}
			else
			{
				delem = next;
				val = 0;
			}
		}
		while (val >= POINTERS_PER_NODE)
		{
			val = delem->node.parent_val;
			delem = delem->node.parent;
			if (delem == NULL)
				break;
			val++;
		}
	}
	STATE_NEXT(state) = NULL;
}

/*
 * mowgli_patricia_find(mowgli_patricia_t *dtree, const char *key)
 *
 * Looks up a DTree node by name.
 *
 * Inputs:
 *     - patricia tree object
 *     - name of node to lookup
 *
 * Outputs:
 *     - on success, the dtree node requested
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
static struct patricia_leaf *mowgli_patricia_find(mowgli_patricia_t *dict, const char *key)
{
	char ckey_store[256];
	char *ckey;
	union patricia_elem *delem;
	int val, keylen;

	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	keylen = strlen(key);
	if (keylen >= sizeof ckey_store)
		ckey = strdup(key);
	else
	{
		strcpy(ckey_store, key);
		ckey = ckey_store;
	}
	dict->canonize_cb(ckey);

	delem = dict->root;
	while (delem != NULL && !IS_LEAF(delem))
	{
		if (delem->nibnum / 2 < keylen)
			val = NIBBLE_VAL(ckey, delem->nibnum);
		else
			val = 0;
		delem = delem->node.down[val];
	}
	/* Now, if the key is in the tree, delem contains it. */
	if (delem != NULL && strcmp(delem->leaf.key, ckey))
		delem = NULL;

	if (ckey != ckey_store)
		free(ckey);

	return &delem->leaf;
}

/*
 * mowgli_patricia_add(mowgli_patricia_t *dtree, const char *key, void *data)
 *
 * Creates a new DTree node and binds data to it.
 *
 * Inputs:
 *     - patricia tree object
 *     - name for new DTree node
 *     - data to bind to the new DTree node
 *
 * Outputs:
 *     - on success, TRUE
 *     - on failure, FALSE
 *
 * Side Effects:
 *     - data is inserted into the DTree.
 */
mowgli_boolean_t mowgli_patricia_add(mowgli_patricia_t *dict, const char *key, void *data)
{
	char *ckey;
	union patricia_elem *delem, *prev, *newnode;
	union patricia_elem **place1;
	int val, keylen;
	int i, j;

	return_val_if_fail(dict != NULL, FALSE);
	return_val_if_fail(key != NULL, FALSE);
	return_val_if_fail(data != NULL, FALSE);

	keylen = strlen(key);
	ckey = strdup(key);
	if (ckey == NULL)
	{
		mowgli_log("major WTF: ckey is NULL, not adding node.");
		return FALSE;
	}
	dict->canonize_cb(ckey);

	prev = NULL;
	val = POINTERS_PER_NODE + 2; /* trap value */
	delem = dict->root;
	while (delem != NULL && !IS_LEAF(delem))
	{
		prev = delem;
		if (delem->nibnum / 2 < keylen)
			val = NIBBLE_VAL(ckey, delem->nibnum);
		else
			val = 0;
		delem = delem->node.down[val];
	}
	/* Now, if the key is in the tree, delem contains it. */
	if (delem != NULL && !strcmp(delem->leaf.key, ckey))
	{
		mowgli_log("Key is already in dict, ignoring duplicate");
		free(ckey);
		return FALSE;
	}

	if (delem == NULL && prev != NULL)
	{
		/* Get a leaf to compare with. */
		delem = first_leaf(prev);
	}

	if (delem == NULL)
	{
		soft_assert(prev == NULL);
		soft_assert(dict->count == 0);
		place1 = &dict->root;
		*place1 = mowgli_heap_alloc(leaf_heap);
		(*place1)->nibnum = -1;
		(*place1)->leaf.data = data;
		(*place1)->leaf.key = ckey;
		(*place1)->leaf.parent = prev;
		(*place1)->leaf.parent_val = val;
		dict->count++;
		return TRUE;
	}

	/* Find the first nibble where they differ. */
	for (i = 0; NIBBLE_VAL(ckey, i) == NIBBLE_VAL(delem->leaf.key, i); i++)
		;
	/* Find where to insert the new node. */
	while (prev != NULL && prev->nibnum > i)
	{
		val = prev->node.parent_val;
		prev = prev->node.parent;
	}
	if (prev == NULL || prev->nibnum < i)
	{
		/* Insert new node below prev */
		newnode = mowgli_heap_alloc(node_heap);
		newnode->nibnum = i;
		newnode->node.parent = prev;
		newnode->node.parent_val = val;
		for (j = 0; j < POINTERS_PER_NODE; j++)
			newnode->node.down[j] = NULL;
		if (prev == NULL)
		{
			newnode->node.down[NIBBLE_VAL(delem->leaf.key, i)] = dict->root;
			if (IS_LEAF(dict->root))
			{
				dict->root->leaf.parent = newnode;
				dict->root->leaf.parent_val = NIBBLE_VAL(delem->leaf.key, i);
			}
			else
			{
				soft_assert(dict->root->nibnum > i);
				dict->root->node.parent = newnode;
				dict->root->node.parent_val = NIBBLE_VAL(delem->leaf.key, i);
			}
			dict->root = newnode;
		}
		else
		{
			newnode->node.down[NIBBLE_VAL(delem->leaf.key, i)] = prev->node.down[val];
			if (IS_LEAF(prev->node.down[val]))
			{
				prev->node.down[val]->leaf.parent = newnode;
				prev->node.down[val]->leaf.parent_val = NIBBLE_VAL(delem->leaf.key, i);
			}
			else
			{
				prev->node.down[val]->node.parent = newnode;
				prev->node.down[val]->node.parent_val = NIBBLE_VAL(delem->leaf.key, i);
			}
			prev->node.down[val] = newnode;
		}
	}
	else
	{
		/* This nibble is already checked. */
		soft_assert(prev->nibnum == i);
		newnode = prev;
	}
	val = NIBBLE_VAL(ckey, i);
	place1 = &newnode->node.down[val];
	soft_assert(*place1 == NULL);
	*place1 = mowgli_heap_alloc(leaf_heap);
	(*place1)->nibnum = -1;
	(*place1)->leaf.data = data;
	(*place1)->leaf.key = ckey;
	(*place1)->leaf.parent = newnode;
	(*place1)->leaf.parent_val = val;

	dict->count++;

	return TRUE;
}

/*
 * mowgli_patricia_delete(mowgli_patricia_t *dtree, const char *key)
 *
 * Deletes data from a patricia tree.
 *
 * Inputs:
 *     - patricia tree object
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
void *mowgli_patricia_delete(mowgli_patricia_t *dict, const char *key)
{
	void *data;
	char ckey_store[256];
	char *ckey;
	union patricia_elem *delem, *prev, *next;
	int val, i, keylen, used;

	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	keylen = strlen(key);

	if (keylen >= sizeof ckey_store)
		ckey = strdup(key);
	else
	{
		strcpy(ckey_store, key);
		ckey = ckey_store;
	}
	dict->canonize_cb(ckey);

	val = POINTERS_PER_NODE + 2; /* trap value */
	delem = dict->root;
	while (delem != NULL && !IS_LEAF(delem))
	{
		if (delem->nibnum / 2 < keylen)
			val = NIBBLE_VAL(ckey, delem->nibnum);
		else
			val = 0;
		delem = delem->node.down[val];
	}
	/* Now, if the key is in the tree, delem contains it. */
	if (delem != NULL && strcmp(delem->leaf.key, ckey))
		delem = NULL;

	if (ckey != ckey_store)
		free(ckey);

	if (delem == NULL)
		return NULL;

	data = delem->leaf.data;

	val = delem->leaf.parent_val;
	prev = delem->leaf.parent;

	mowgli_free(delem->leaf.key);
	mowgli_heap_free(leaf_heap, delem);

	if (prev != NULL)
	{
		prev->node.down[val] = NULL;

		/* Leaf is gone, now consider the node it was in. */
		delem = prev;

		used = -1;
		for (i = 0; i < POINTERS_PER_NODE; i++)
			if (delem->node.down[i] != NULL)
				used = used == -1 ? i : -2;
		soft_assert(used == -2 || used >= 0);
		if (used >= 0)
		{
			/* Only one pointer in this node, remove it.
			 * Replace the pointer that pointed to it by
			 * the sole pointer in it.
			 */
			next = delem->node.down[used];
			val = delem->node.parent_val;
			prev = delem->node.parent;
			if (prev != NULL)
				prev->node.down[val] = next;
			else
				dict->root = next;
			if (IS_LEAF(next))
				next->leaf.parent = prev, next->leaf.parent_val = val;
			else
				next->node.parent = prev, next->node.parent_val = val;
			mowgli_heap_free(node_heap, delem);
		}
	}
	else
	{
		/* This was the last leaf. */
		dict->root = NULL;
	}

	dict->count--;
	if (dict->count == 0)
	{
		soft_assert(dict->root == NULL);
		dict->root = NULL;
	}

	return data;
}

/*
 * mowgli_patricia_retrieve(mowgli_patricia_t *dtree, const char *key)
 *
 * Retrieves data from a patricia.
 *
 * Inputs:
 *     - patricia tree object
 *     - name of node to lookup
 *
 * Outputs:
 *     - on success, the data bound to the DTree node.
 *     - on failure, NULL
 *
 * Side Effects:
 *     - none
 */
void *mowgli_patricia_retrieve(mowgli_patricia_t *dtree, const char *key)
{
	struct patricia_leaf *delem = mowgli_patricia_find(dtree, key);

	if (delem != NULL)
		return delem->data;

	return NULL;
}

/*
 * mowgli_patricia_size(mowgli_patricia_t *dict)
 *
 * Returns the size of a patricia.
 *
 * Inputs:
 *     - patricia tree object
 *
 * Outputs:
 *     - size of patricia
 *
 * Side Effects:
 *     - none
 */
unsigned int mowgli_patricia_size(mowgli_patricia_t *dict)
{
	return_val_if_fail(dict != NULL, 0);

	return dict->count;
}

/* returns the sum of the depths of the subtree rooted in delem at depth depth */
/* there is no need for this to be recursive, but it is easier... */
static int
stats_recurse(union patricia_elem *delem, int depth, int *pmaxdepth)
{
	int result = 0;
	int val;
	union patricia_elem *next;

	if (depth > *pmaxdepth)
		*pmaxdepth = depth;
	if (depth == 0)
	{
		if (IS_LEAF(delem))
		{
			soft_assert(delem->leaf.parent == NULL);
		}
		else
		{
			soft_assert(delem->node.parent == NULL);
		}
	}
	if (IS_LEAF(delem))
		return depth;
	for (val = 0; val < POINTERS_PER_NODE; val++)
	{
		next = delem->node.down[val];
		if (next == NULL)
			continue;
		result += stats_recurse(next, depth + 1, pmaxdepth);
		if (IS_LEAF(next))
		{
			soft_assert(next->leaf.parent == delem);
			soft_assert(next->leaf.parent_val == val);
		}
		else
		{
			soft_assert(next->node.parent == delem);
			soft_assert(next->node.parent_val == val);
			soft_assert(next->node.nibnum > delem->node.nibnum);
		}
	}
	return result;
}

/*
 * mowgli_patricia_stats(mowgli_patricia_t *dict, void (*cb)(const char *line, void *privdata), void *privdata)
 *
 * Returns the size of a patricia.
 *
 * Inputs:
 *     - patricia tree object
 *     - callback
 *     - data for callback
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - callback called with stats text
 */
void mowgli_patricia_stats(mowgli_patricia_t *dict, void (*cb)(const char *line, void *privdata), void *privdata)
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
	if (dict->count > 0)
	{
		sum = stats_recurse(dict->root, 0, &maxdepth);
		snprintf(str, sizeof str, "Depth sum %d Avg depth %d Max depth %d", sum, sum / dict->count, maxdepth);
	}
	else
		snprintf(str, sizeof str, "Depth sum 0 Avg depth 0 Max depth 0");
	cb(str, privdata);
	return;
}
