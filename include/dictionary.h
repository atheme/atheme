/*
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * A simple dictionary tree implementation.
 * See Knuth ACP, volume 1 for a more detailed explanation.
 *
 * $Id: dictionary.h 5983 2006-08-01 00:36:17Z nenolod $
 */

#ifndef _DICTIONARY_H
#define _DICTIONARY_H

struct dictionary_tree_
{
	int resolution;
	list_t *hashv;		/* dynamically allocated by dictionary_create() */
	int (*compare_cb)(const char *a, const char *b);
};

typedef struct dictionary_tree_ dictionary_tree_t;

/*
 * dictionary_elem_t is a child of node_t, which adds a key field.
 * node_t access is done by ((node_t *) delem)->next|prev,
 * or through delem->node.(next|prev).
 */
struct dictionary_elem_
{
	node_t node;
	char *key;
};

typedef struct dictionary_elem_ dictionary_elem_t;

/*
 * dictionary_create() creates a new dictionary tree of the defined resolution.
 */
E dictionary_tree_t *dictionary_create(int resolution);

/*
 * dictionary_destroy() destroys all entries in a dtree, and also optionally calls
 * a defined callback function to destroy any data attached to it.
 */
E void dictionary_destroy(dictionary_tree_t *dtree,
	void (*destroy_cb)(dictionary_elem_t *delem, void *privdata),
	void *privdata);

/*
 * dictionary_foreach() iterates all entries in a dtree, and also optionally calls
 * a defined callback function to use any data attached to it.
 *
 * To shortcircuit iteration, return non-zero from the callback function.
 */
E void dictionary_foreach(dictionary_tree_t *dtree,
	int (*foreach_cb)(dictionary_elem_t *delem, void *privdata),
	void *privdata);

/*
 * dictionary_search() iterates all entries in a dtree, and also optionally calls
 * a defined callback function to use any data attached to it.
 *
 * When the object is found, a non-NULL is returned from the callback, which results
 * in that object being returned to the user.
 */
E void *dictionary_search(dictionary_tree_t *dtree,
	void *(*foreach_cb)(dictionary_elem_t *delem, void *privdata),
	void *privdata);

/*
 * dictionary_add() adds a key->value entry to the dictionary tree.
 */
E dictionary_elem_t *dictionary_add(dictionary_tree_t *dtree, char *key, void *data);

/*
 * dictionary_find() returns a dictionary_elem_t container from a dtree for key 'key'.
 */
E dictionary_elem_t *dictionary_find(dictionary_tree_t *dtree, char *key);

/*
 * dictionary_find() returns data from a dtree for key 'key'.
 */
E void *dictionary_retrieve(dictionary_tree_t *dtree, char *key);

/*
 * dictionary_delete() deletes a key->value entry from the dictionary tree.
 */
E void *dictionary_delete(dictionary_tree_t *dtree, char *key);

/*
 * dictionary_set_compare_func() changes the key-based indexing method used.
 * Typically, either strcasecmp or irccasecmp is used here.
 */
E void dictionary_set_compare_func(dictionary_tree_t *dtree,
	int (*compare_cb)(const char *a, const char *b));

#endif
