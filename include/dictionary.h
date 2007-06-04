/*
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * A simple dictionary tree implementation.
 * See Knuth ACP, volume 1 for a more detailed explanation.
 *
 * $Id: dictionary.h 8413 2007-06-04 18:45:05Z pippijn $
 */

#ifndef _DICTIONARY_H
#define _DICTIONARY_H

struct dictionary_tree_; /* defined in src/dictionary.c */

typedef struct dictionary_tree_ dictionary_tree_t;

/*
 * dictionary_elem_t is a child of node_t, which adds a key field.
 * node_t access is done by ((node_t *) delem)->next|prev,
 * or through delem->node.(next|prev).
 */
struct dictionary_elem_
{
	node_t node;
	const char *key;
};

typedef struct dictionary_elem_ dictionary_elem_t;

/*
 * dictionary_iteration_state_t, private.
 */
struct dictionary_iteration_state_
{
	int bucket;
	dictionary_elem_t *cur, *next;
};

typedef struct dictionary_iteration_state_ dictionary_iteration_state_t;

#define DICTIONARY_FOREACH(element, state, dict) for (dictionary_foreach_start((dict), (state)); (element = dictionary_foreach_cur((dict), (state))); dictionary_foreach_next((dict), (state)))

/*
 * dictionary_create() creates a new dictionary tree of the defined resolution.
 * name is only used for statistical purposes.
 * compare_cb is the comparison function, typically strcmp, strcasecmp or
 * irccasecmp.
 */
E dictionary_tree_t *dictionary_create(const char *name, int resolution, int (*compare_cb)(const char *a, const char *b));

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
 * dictionary_foreach_start() begins an iteration over all items
 * keeping state in the given struct. If there is only one iteration
 * in progress at a time, it is permitted to remove the current element
 * of the iteration (but not any other element).
 */
E void dictionary_foreach_start(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state);

/*
 * dictionary_foreach_cur() returns the current element of the iteration,
 * or NULL if there are no more elements.
 */
E void *dictionary_foreach_cur(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state);

/*
 * dictionary_foreach_next() moves to the next element.
 */
E void dictionary_foreach_next(dictionary_tree_t *dtree,
	dictionary_iteration_state_t *state);

/*
 * dictionary_add() adds a key->value entry to the dictionary tree.
 */
E dictionary_elem_t *dictionary_add(dictionary_tree_t *dtree, const char *key, void *data);

/*
 * dictionary_find() returns a dictionary_elem_t container from a dtree for key 'key'.
 */
E dictionary_elem_t *dictionary_find(dictionary_tree_t *dtree, const char *key);

/*
 * dictionary_retrieve() returns data from a dtree for key 'key'.
 */
E void *dictionary_retrieve(dictionary_tree_t *dtree, const char *key);

/*
 * dictionary_delete() deletes a key->value entry from the dictionary tree.
 */
E void *dictionary_delete(dictionary_tree_t *dtree, const char *key);

/*
 * dictionary_stats() outputs hash statistics for all dictionary trees.
 * The output consists of a number of lines, output one by one via the
 * given callback.
 */
E void dictionary_stats(void (*stats_cb)(const char *line, void *privdata),
		void *privdata);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
