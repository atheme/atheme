/*
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple dictionary tree implementation.
 * See Knuth ACP, volume 1 for a more detailed explanation.
 *
 * $Id: dictionary.c 7487 2007-01-14 08:34:12Z nenolod $
 */

#include "atheme.h"

list_t dictionarylist;

struct dictionary_tree_
{
	object_t parent;

	int resolution;
	list_t *hashv;		/* dynamically allocated by dictionary_create() */
	int (*compare_cb)(const char *a, const char *b);
	node_t node;
};

/*
 * dictionary_create(const char *name, int resolution,
 *                   int (*compare_cb)(const char *a, const char *b)
 *
 * DTree object factory.
 *
 * Inputs:
 *     - name of dictionary tree
 *     - resolution of dictionary tree
 *     - function to use for comparing two entries in the dtree
 *
 * Outputs:
 *     - on success, a new DTree object.
 *
 * Side Effects:
 *     - if services runs out of memory and cannot allocate the object,
 *       the program will abort.
 */
dictionary_tree_t *dictionary_create(const char *name, int resolution, int (*compare_cb)(const char *a, const char *b))
{
	dictionary_tree_t *dtree = smalloc(sizeof(dictionary_tree_t));

	object_init(&dtree->parent, name, NULL);
	dtree->resolution = resolution;
	dtree->hashv      = smalloc(sizeof(list_t) * resolution);
	dtree->compare_cb = compare_cb;
	memset(dtree->hashv, '\0', sizeof(list_t) * resolution);
	node_add(dtree, &dtree->node, &dictionarylist);

	return dtree;
}

/*
 * dictionary_destroy(dictionary_tree_t *dtree,
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
 *
 * Bugs:
 *     - this function assumes that the given dtree reference is valid.
 */
void dictionary_destroy(dictionary_tree_t *dtree,
	void (*destroy_cb)(dictionary_elem_t *delem, void *privdata),
	void *privdata)
{
	node_t *n, *tn;
	int i;

	for (i = 0; i < dtree->resolution; i++)
	{
		LIST_FOREACH_SAFE(n, tn, dtree->hashv[i].head)
		{
			/* delem_t is a subclass of node_t. */
			dictionary_elem_t *delem = (dictionary_elem_t *) n;

			if (destroy_cb != NULL)
				(*destroy_cb)(delem, privdata);

			node_del(&delem->node, &dtree->hashv[i]);

			free(delem);
		}
	}

	node_del(&dtree->node, &dictionarylist);

	free(dtree->hashv);
	free(dtree);
}

/*
 * dictionary_foreach(dictionary_tree_t *dtree,
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
 *
 * Bugs:
 *     - this function assumes that the given dtree reference is valid.
 */
void dictionary_foreach(dictionary_tree_t *dtree,
	int (*foreach_cb)(dictionary_elem_t *delem, void *privdata),
	void *privdata)
{
	node_t *n, *tn;
	int i, ret = 0;

	for (i = 0; i < dtree->resolution && ret == 0; i++)
	{
		LIST_FOREACH_SAFE(n, tn, dtree->hashv[i].head)
		{
			/* delem_t is a subclass of node_t. */
			dictionary_elem_t *delem = (dictionary_elem_t *) n;

			if (foreach_cb != NULL)
				ret = (*foreach_cb)(delem, privdata);
		}
	}
}

/*
 * dictionary_foreach(dictionary_tree_t *dtree,
 *     void (*destroy_cb)(dictionary_elem_t *delem, void *privdata),
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
 *
 * Bugs:
 *     - this function assumes that the given dtree reference is valid.
 */
void *dictionary_search(dictionary_tree_t *dtree,
	void *(*foreach_cb)(dictionary_elem_t *delem, void *privdata),
	void *privdata)
{
	node_t *n, *tn;
	int i;
	void *ret = NULL;

	for (i = 0; i < dtree->resolution && ret == NULL; i++)
	{
		LIST_FOREACH_SAFE(n, tn, dtree->hashv[i].head)
		{
			/* delem_t is a subclass of node_t. */
			dictionary_elem_t *delem = (dictionary_elem_t *) n;

			if (foreach_cb != NULL)
				ret = (*foreach_cb)(delem, privdata);
		}
	}

	return ret;
}

/*
 * dictionary_foreach_start(dictionary_tree_t *dtree,
 *     dictionary_iteration_state_t *state);
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
 *
 * Bugs:
 *     - this function assumes that the given dtree reference is valid.
 */
void dictionary_foreach_start(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state)
{
	state->bucket = 0;
	state->cur = NULL;
	state->next = NULL;
	/* find first item */
	while (state->bucket < dtree->resolution)
	{
		state->cur = (dictionary_elem_t *)dtree->hashv[state->bucket].head;
		if (state->cur != NULL)
			break;
		state->bucket++;
	}
	if (state->cur == NULL)
		return;
	/* make state->cur point to first item and state->next point to
	 * second item */
	state->next = state->cur;
	dictionary_foreach_next(dtree, state);
}

/*
 * dictionary_foreach_cur(dictionary_tree_t *dtree,
 *     dictionary_iteration_state_t *state);
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
 *
 * Bugs:
 *     - this function assumes that the given dtree reference is valid.
 */
void *dictionary_foreach_cur(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state)
{
	return state->cur != NULL ? state->cur->node.data : NULL;
}

/*
 * dictionary_foreach_next(dictionary_tree_t *dtree,
 *     dictionary_iteration_state_t *state);
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
 *
 * Bugs:
 *     - this function assumes that the given dtree reference is valid.
 */
void dictionary_foreach_next(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state)
{
	if (state->cur == NULL)
	{
		slog(LG_DEBUG, "dictionary_foreach_next(): called again after iteration finished on dtree<%p>", dtree);
		return;
	}
	state->cur = state->next;
	if (state->next == NULL)
		return;
	state->next = (dictionary_elem_t *)state->next->node.next;
	if (state->next != NULL)
		return;
	while (++state->bucket < dtree->resolution)
	{
		state->next = (dictionary_elem_t *)dtree->hashv[state->bucket].head;
		if (state->next != NULL)
			return;
	}
}

/*
 * dictionary_find(dictionary_tree_t *dtree, const char *key)
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
dictionary_elem_t *dictionary_find(dictionary_tree_t *dtree, const char *key)
{
	node_t *n;
	int i;

	if (dtree == NULL || key == NULL)
		return NULL;

	i = shash((const unsigned char *) key) % dtree->resolution;

	LIST_FOREACH(n, dtree->hashv[i].head)
	{
		/* delem_t is a subclass of node_t. */
		dictionary_elem_t *delem = (dictionary_elem_t *) n;

		if (!dtree->compare_cb(key, delem->key))
			return delem;
	}

	return NULL;
}

/*
 * dictionary_add(dictionary_tree_t *dtree, const char *key, void *data)
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
dictionary_elem_t *dictionary_add(dictionary_tree_t *dtree, const char *key, void *data)
{
	dictionary_elem_t *delem;
	int i;

	if (dtree == NULL || key == NULL || data == NULL)
		return NULL;

	if (dictionary_find(dtree, key) != NULL)
	{
		slog(LG_DEBUG, "dictionary_add(): entry already exists in dtree<%p> for key '%s'!",
			dtree, key);
		return NULL;
	}

	i = shash((const unsigned char *) key) % dtree->resolution;
	delem = smalloc(sizeof(dictionary_elem_t));
	memset(delem, '\0', sizeof(dictionary_elem_t));

	delem->key = sstrdup(key);
	node_add(data, &delem->node, &dtree->hashv[i]);

	return delem;
}

/*
 * dictionary_delete(dictionary_tree_t *dtree, const char *key)
 *
 * Deletes data from a dictionary tree.
 *
 * Inputs:
 *     - dictionary tree object
 *     - name of DTree node to delete
 *
 * Outputs:
 *     - on success, the remaining data that needs to be freed
 *     - on failure, NULL
 *
 * Side Effects:
 *     - data is removed from the DTree.
 *
 * Notes:
 *     - the returned data needs to be freed/released manually!
 */
void *dictionary_delete(dictionary_tree_t *dtree, const char *key)
{
	dictionary_elem_t *delem = dictionary_find(dtree, key);
	void *data;
	int i;

	if (delem == NULL)
	{
		slog(LG_DEBUG, "dictionary_delete(): entry '%s' does not exist in dtree<%p>!",
			key, dtree);
		return NULL;
	}

	i = shash((const unsigned char *) key) % dtree->resolution;

	data = delem->node.data;

	node_del(&delem->node, &dtree->hashv[i]);

	free(delem);	

	return data;
}

/*
 * dictionary_retrieves(dictionary_tree_t *dtree, const char *key)
 *
 * Retrieves data from a dictionary tree.
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
void *dictionary_retrieve(dictionary_tree_t *dtree, const char *key)
{
	dictionary_elem_t *delem = dictionary_find(dtree, key);

	if (delem != NULL)
		return delem->node.data;

	return NULL;
}

#define MAXCOUNT 10

/*
 * dictionary_stats(void (*stats_cb)(const char *line, void *privdata), void *privdata)
 *
 * Displays statistics about all of the registered DTrees.
 *
 * Inputs:
 *     - callback function
 *     - optional opaque data
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - data is passed to the callback function about the dtree system.
 */
void dictionary_stats(void (*stats_cb)(const char *line, void *privdata), void *privdata)
{
	node_t *n;
	dictionary_tree_t *dtree;
	char buf[120];
	int i, count1, totalcount, maxdepth;
	int counts[MAXCOUNT + 1];

	LIST_FOREACH(n, dictionarylist.head)
	{
		dtree = object_ref(n->data);
		snprintf(buf, sizeof buf, "Hash statistics for %s", object(dtree)->name);
		stats_cb(buf, privdata);
		for (i = 0; i <= MAXCOUNT; i++)
			counts[i] = 0;
		totalcount = 0;
		maxdepth = 0;
		for (i = 0; i < dtree->resolution; i++)
		{
			count1 = LIST_LENGTH(&dtree->hashv[i]);
			totalcount += count1;
			if (count1 > maxdepth)
				maxdepth = count1;
			if (count1 > MAXCOUNT)
				count1 = MAXCOUNT;
			counts[count1]++;
		}
		snprintf(buf, sizeof buf, "Size: %d  Items: %d  Max depth: %d",
				dtree->resolution, totalcount, maxdepth);
		stats_cb(buf, privdata);
		for (i = 0; i <= MAXCOUNT; i++)
		{
			snprintf(buf, sizeof buf, "Nodes with %d%s entries: %d",
					i, i == MAXCOUNT ? " or more" : "",
					counts[i]);
			stats_cb(buf, privdata);
		}
		object_unref(dtree);
	}
}
