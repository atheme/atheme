/*
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple dictionary tree implementation.
 * See Knuth ACP, volume 1 for a more detailed explanation.
 *
 * $Id: dictionary.c 5997 2006-08-01 20:41:37Z jilles $
 */

#include "atheme.h"

struct dictionary_tree_
{
	int resolution;
	list_t *hashv;		/* dynamically allocated by dictionary_create() */
	int (*compare_cb)(const char *a, const char *b);
};

dictionary_tree_t *dictionary_create(int resolution, int (*compare_cb)(const char *a, const char *b))
{
	dictionary_tree_t *dtree = smalloc(sizeof(dictionary_tree_t));

	dtree->resolution = resolution;
	dtree->hashv      = smalloc(sizeof(list_t) * resolution);
	dtree->compare_cb = compare_cb;
	memset(dtree->hashv, '\0', sizeof(list_t) * resolution);

	return dtree;
}

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

			if (delem->key != NULL)
				free(delem->key);

			node_del(&delem->node, &dtree->hashv[i]);

			free(delem);
		}
	}

	free(dtree->hashv);
	free(dtree);
}

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

void *dictionary_foreach_cur(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state)
{
	return state->cur != NULL ? state->cur->node.data : NULL;
}

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

dictionary_elem_t *dictionary_find(dictionary_tree_t *dtree, char *key)
{
	node_t *n;
	int i;

	if (dtree == NULL || key == NULL)
		return NULL;

	i = shash((unsigned char *) key) % dtree->resolution;

	LIST_FOREACH(n, dtree->hashv[i].head)
	{
		/* delem_t is a subclass of node_t. */
		dictionary_elem_t *delem = (dictionary_elem_t *) n;

		if (!dtree->compare_cb(key, delem->key))
			return delem;
	}

	return NULL;
}

dictionary_elem_t *dictionary_add(dictionary_tree_t *dtree, char *key, void *data)
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

	i = shash((unsigned char *) key) % dtree->resolution;
	delem = smalloc(sizeof(dictionary_elem_t));
	memset(delem, '\0', sizeof(dictionary_elem_t));

	delem->key = sstrdup(key);
	node_add(data, &delem->node, &dtree->hashv[i]);

	return delem;
}

void *dictionary_delete(dictionary_tree_t *dtree, char *key)
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

	i = shash((unsigned char *) key) % dtree->resolution;

	data = delem->node.data;

	node_del(&delem->node, &dtree->hashv[i]);

	free(delem);	

	return data;
}

void *dictionary_retrieve(dictionary_tree_t *dtree, char *key)
{
	dictionary_elem_t *delem = dictionary_find(dtree, key);

	if (delem == NULL)
	{
		slog(LG_DEBUG, "dictionary_retrieve(): entry '%s' does not exist in dtree<%p>!",
			key, dtree);
		return NULL;
	}
	else
		return delem->node.data;
}
