/*
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A simple dictionary tree implementation.
 * See Knuth ACP, volume 1 for a more detailed explanation.
 *
 * $Id: dictionary.c 5899 2006-07-17 20:09:26Z nenolod $
 */

#include "atheme.h"

dictionary_tree_t *dictionary_create(int resolution)
{
	dictionary_tree_t *dtree = smalloc(sizeof(dictionary_tree_t));

	dtree->resolution = resolution;
	dtree->hashv      = smalloc(sizeof(list_t) * resolution);
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

		if (!strcasecmp(key, delem->key))
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
