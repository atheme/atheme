/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_patricia.c: Dictionary-based information storage.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007-2008 Jilles Tjoelker <jilles -at- stack.nl>
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

static mowgli_heap_t *elem_heap = NULL;

/*
 * Patricia tree.
 *
 * A radix trie that avoids one-way branching and redundant nodes.
 *
 * To find a node, the tree is traversed starting from the root. The
 * bitnum in each node indicates which bit of the key needs to be
 * tested, and the appropriate branch is taken. The keys in the nodes
 * are not used during this descent.
 *
 * The bitnum values are strictly increasing while going down the tree.
 * If the node pointed to has a lower or equal bitnum (an upward link),
 * the key in this node is the only one that could match the requested key.
 *
 * When adding an entry, a node is inserted at the lowest bitnum where
 * the key in the found node and the requested key differ. The new node
 * contains the requested key and has an upward link to itself, while its
 * other branch leads to the found node. Note that any other keys in that
 * subtree will be equal on the new node's bitnum.
 *
 * When removing an entry, the node that has the upward link to the
 * node with the requested key needs to be removed from the tree, because
 * that bit test is no longer needed. If these two are the same node, it is
 * simple; otherwise the node with the upward link needs to be moved to the
 * node with the requested key.
 *
 * -- jilles
 */

struct mowgli_patricia_
{
	void (*canonize_cb)(char *key);
	mowgli_patricia_elem_t *root;
	/* head and tail of linked list */
	mowgli_patricia_elem_t *head, *tail;
	unsigned int count;
	char *id;
};

struct mowgli_patricia_elem_
{
	/* bit number to test on (bit NUM%8 of byte NUM/8) */
	int bitnum;
	/* branches of the tree */
	mowgli_patricia_elem_t *zero, *one;
	/* linked list */
	mowgli_patricia_elem_t *next, *prev;
	/* data associated with the key */
	void *data;
	/* key (canonized copy) */
	char *key;
};

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

	if (!elem_heap)
		elem_heap = mowgli_heap_create(sizeof(mowgli_patricia_elem_t), 1024, BH_NOW);

	dtree->root = NULL;
	dtree->head = NULL;
	dtree->tail = NULL;

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

	if (!elem_heap)
		elem_heap = mowgli_heap_create(sizeof(mowgli_patricia_elem_t), 1024, BH_NOW);

	dtree->root = NULL;
	dtree->head = NULL;
	dtree->tail = NULL;

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
	mowgli_patricia_elem_t *n, *tn;

	return_if_fail(dtree != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, dtree->head)
	{
		if (destroy_cb != NULL)
			(*destroy_cb)(n->key, n->data, privdata);

		mowgli_free(n->key);
		mowgli_heap_free(elem_heap, n);
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
	mowgli_patricia_elem_t *delem, *tn;

	return_if_fail(dtree != NULL);

	MOWGLI_LIST_FOREACH_SAFE(delem, tn, dtree->head)
	{
		if (foreach_cb != NULL)
			(*foreach_cb)(delem->key, delem->data, privdata);
	}
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
	mowgli_patricia_elem_t *delem, *tn;
	void *ret = NULL;

	return_val_if_fail(dtree != NULL, NULL);

	MOWGLI_LIST_FOREACH_SAFE(delem, tn, dtree->head)
	{
		if (foreach_cb != NULL)
			ret = (*foreach_cb)(delem->key, delem->data, privdata);

		if (ret)
			break;
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

	state->cur = NULL;
	state->next = NULL;

	/* find first item */
	state->cur = dtree->head;

	if (state->cur == NULL)
		return;

	/* make state->cur point to first item and state->next point to
	 * second item */
	state->next = state->cur;
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

	return state->cur != NULL ? state->cur->data : NULL;
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
	return_if_fail(dtree != NULL);
	return_if_fail(state != NULL);

	if (state->cur == NULL)
	{
		mowgli_log("mowgli_patricia_foreach_next(): called again after iteration finished on dtree<%p>", dtree);
		return;
	}

	state->cur = state->next;

	if (state->next == NULL)
		return;

	state->next = state->next->next;
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
static mowgli_patricia_elem_t *mowgli_patricia_find(mowgli_patricia_t *dict, const char *key)
{
	char ckey_store[256];
	char *ckey;
	mowgli_patricia_elem_t *delem, *prev;
	int bitval, keylen;

	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	if (dict->root == NULL)
		return NULL;

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
	do
	{
		prev = delem;
		if (delem->bitnum / 8 < keylen)
			bitval = (ckey[delem->bitnum / 8] & (1 << (delem->bitnum & 7))) != 0;
		else
			bitval = 0;
		delem = bitval ? delem->one : delem->zero;
	} while (delem != NULL && prev->bitnum < delem->bitnum);
	/* Now, if the key is in the tree, delem contains it. */
	if (delem != NULL && strcmp(delem->key, ckey))
		delem = NULL;

	if (ckey != ckey_store)
		free(ckey);

	return delem;
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
	mowgli_patricia_elem_t *delem, *prev, *place, *newelem;
	int bitval, keylen;
	int i;

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

	if (dict->root == NULL)
	{
		return_val_if_fail(dict->count == 0, FALSE);
		return_val_if_fail(dict->head == NULL, FALSE);
		return_val_if_fail(dict->tail == NULL, FALSE);
		dict->root = mowgli_heap_alloc(elem_heap);
		dict->root->bitnum = 0;
		if (ckey[0] & 1)
		{
			dict->root->zero = NULL;
			dict->root->one = dict->root;
		}
		else
		{
			dict->root->zero = dict->root;
			dict->root->one = NULL;
		}
		dict->root->next = NULL;
		dict->root->prev = NULL;
		dict->root->data = data;
		dict->root->key = ckey;
		dict->head = dict->root;
		dict->tail = dict->root;
		dict->count++;
		return TRUE;
	}

	delem = dict->root;
	do
	{
		prev = delem;
		if (delem->bitnum / 8 < keylen)
			bitval = (ckey[delem->bitnum / 8] & (1 << (delem->bitnum & 7))) != 0;
		else
			bitval = 0;
		delem = bitval ? delem->one : delem->zero;
	} while (delem != NULL && prev->bitnum < delem->bitnum);
	/* Now, if the key is in the tree, delem contains it. */
	if (delem != NULL && !strcmp(delem->key, ckey))
	{
		mowgli_log("Key is already in dict, ignoring duplicate");
		free(ckey);
		return FALSE;
	}
	place = delem;

	/* Find the first bit position where they differ */
	/* XXX perhaps look up differing byte first, then bit */
	if (place == NULL)
		i = prev->bitnum + 1;
	else
		for (i = 0; !((ckey[i / 8] ^ place->key[i / 8]) & (1 << (i & 7))); i++)
			;
	/* Find where to insert the new node */
	prev = NULL;
	delem = dict->root;
	while ((prev == NULL || prev->bitnum < delem->bitnum) &&
			delem->bitnum < i)
	{
		prev = delem;
		if (delem->bitnum / 8 < keylen)
			bitval = (ckey[delem->bitnum / 8] & (1 << (delem->bitnum & 7))) != 0;
		else
			bitval = 0;
		delem = bitval ? delem->one : delem->zero;
		if (delem == NULL)
			break;
	}
	soft_assert(delem == NULL || delem->bitnum != i);

	/* Insert new element between prev and delem */
	newelem = mowgli_heap_alloc(elem_heap);
	newelem->bitnum = i;
	newelem->key = ckey;
	newelem->data = data;
	if (prev == NULL)
	{
		soft_assert(dict->root == delem);
		dict->root = newelem;
	}
	else if (bitval)
	{
		soft_assert(prev->one == delem);
		prev->one = newelem;
	}
	else
	{
		soft_assert(prev->zero == delem);
		prev->zero = newelem;
	}
	bitval = (ckey[i / 8] & (1 << (i & 7))) != 0;
	if (bitval)
		newelem->one = newelem, newelem->zero = delem;
	else
		newelem->zero = newelem, newelem->one = delem;

	/* linked list - XXX still needed? */
	if (place != NULL && place->next != NULL)
	{
		newelem->next = place->next;
		newelem->prev = place;
		place->next->prev = newelem;
		place->next = newelem;
	}
	else
	{
		/* XXX head or tail? */
		newelem->next = NULL;
		newelem->prev = dict->tail;
		if (dict->tail != NULL)
			dict->tail->next = newelem;
		dict->tail = newelem;
		if (dict->head == NULL)
			dict->head = newelem;
	}

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
	mowgli_patricia_elem_t *delem, *prev, *pprev, *pdelem, *temp2;
	int bitval, pbitval, keylen;

	return_val_if_fail(dict != NULL, NULL);
	return_val_if_fail(key != NULL, NULL);

	if (dict->root == NULL)
		return NULL;

	keylen = strlen(key);

	if (keylen >= sizeof ckey_store)
		ckey = strdup(key);
	else
	{
		strcpy(ckey_store, key);
		ckey = ckey_store;
	}
	dict->canonize_cb(ckey);

	prev = NULL;
	delem = dict->root;
	do
	{
		pprev = prev;
		prev = delem;
		if (delem->bitnum / 8 < keylen)
			bitval = (ckey[delem->bitnum / 8] & (1 << (delem->bitnum & 7))) != 0;
		else
			bitval = 0;
		delem = bitval ? delem->one : delem->zero;
	} while (delem != NULL && prev->bitnum < delem->bitnum);
	/* Now, if the key is in the tree, delem contains it. */
	if (delem != NULL && strcmp(delem->key, ckey))
		delem = NULL;

	if (ckey != ckey_store)
		free(ckey);

	if (delem == NULL)
		return NULL;

	data = delem->data;

	/* delem = node with requested key in it
	 * prev = node with upward link to delem
	 * pprev = node with downward link to prev
	 */
	if (delem == prev)
	{
		/* The node with the requested key is also the node
		 * that needs to be removed from the tree.
		 */
		/* Get the other pointer of the node to be removed. */
		temp2 = bitval ? prev->zero : prev->one;
		/* Disconnect it. */
		if (pprev == NULL)
			dict->root = temp2;
		else
		{
			if (pprev->zero == prev)
				pprev->zero = temp2;
			if (pprev->one == prev)
				pprev->one = temp2;
		}
	}
	else
	{
		/* Now we need to remove prev from the tree.
		 * However, the node *prev must remain valid and containing
		 * the same key and data, otherwise iterations doing removal
		 * break. We will put it on delem's place.
		 */

		pdelem = NULL;
		temp2 = dict->root;
		while (temp2 != delem)
		{
			pdelem = temp2;
			if (temp2->bitnum / 8 < keylen)
				pbitval = (ckey[temp2->bitnum / 8] & (1 << (temp2->bitnum & 7))) != 0;
			else
				pbitval = 0;
			temp2 = pbitval ? temp2->one : temp2->zero;
		}
		/* pdelem = node containing downward link to delem or
		 *          NULL if delem == dict->root
		 */

		soft_assert((bitval ? prev->one : prev->zero) == delem);
		/* Get the other pointer of the node to be removed. */
		temp2 = bitval ? prev->zero : prev->one;
		/* Disconnect it. */
		if (pprev == NULL)
			dict->root = temp2;
		else
		{
			if (pprev->zero == prev)
				pprev->zero = temp2;
			if (pprev->one == prev)
				pprev->one = temp2;
		}
		/* Make prev take delem's place in the tree. */
		if (pdelem == NULL)
			dict->root = prev;
		else
		{
			if (pdelem->zero == delem)
				pdelem->zero = prev;
			if (pdelem->one == delem)
				pdelem->one = prev;
		}
		prev->one = delem->one;
		prev->zero = delem->zero;
		prev->bitnum = delem->bitnum;
	}

	/* linked list */
#if 0
	if (dict->head == delem)
		dict->head = delem->next;
	else
		delem->prev->next = delem->next;
	if (dict->tail == delem)
		dict->tail = delem->prev;
	else
		delem->next->prev = delem->prev;
#endif
	if (delem->prev == NULL)
		dict->head = delem->next;
	else
		delem->prev->next = delem->next;
	if (delem->next == NULL)
		dict->tail = delem->prev;
	else
		delem->next->prev = delem->prev;

	mowgli_free(delem->key);
	mowgli_heap_free(elem_heap, delem);

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
	mowgli_patricia_elem_t *delem = mowgli_patricia_find(dtree, key);

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
static int
stats_recurse(mowgli_patricia_elem_t *delem, int depth, int *pmaxdepth)
{
	int result = 0;

	if (depth > *pmaxdepth)
		*pmaxdepth = depth;
	if (delem->zero)
	{
		if (delem->zero->bitnum > delem->bitnum)
			result += stats_recurse(delem->zero, depth + 1, pmaxdepth);
		else if (delem->zero->key[0] != '\0')
			result += depth + 1;
	}
	if (delem->one)
	{
		if (delem->one->bitnum > delem->bitnum)
			result += stats_recurse(delem->one, depth + 1, pmaxdepth);
		else if (delem->one->key[0] != '\0')
			result += depth + 1;
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
