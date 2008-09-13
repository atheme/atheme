/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_dictionary.h: Dictionary-based storage.
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

#ifndef __MOWGLI_DICTIONARY_H__
#define __MOWGLI_DICTIONARY_H__

struct mowgli_dictionary_; /* defined in src/dictionary.c */

typedef struct mowgli_dictionary_ mowgli_dictionary_t;

typedef struct mowgli_dictionary_elem_ mowgli_dictionary_elem_t;

typedef int (*mowgli_dictionary_comparator_func_t)(const char *a, const char *b);

struct mowgli_dictionary_elem_
{
	mowgli_dictionary_elem_t *left, *right, *prev, *next;
	void *data;
	char *key;
	int position;
};

/*
 * mowgli_dictionary_iteration_state_t, private.
 */
struct mowgli_dictionary_iteration_state_
{
	mowgli_dictionary_elem_t *cur, *next;
	void *pspare[4];
	int ispare[4];
};

typedef struct mowgli_dictionary_iteration_state_ mowgli_dictionary_iteration_state_t;

/*
 * this is a convenience macro for inlining iteration of dictionaries.
 */
#define MOWGLI_DICTIONARY_FOREACH(element, state, dict) for (mowgli_dictionary_foreach_start((dict), (state)); (element = mowgli_dictionary_foreach_cur((dict), (state))); mowgli_dictionary_foreach_next((dict), (state)))

/*
 * mowgli_dictionary_create() creates a new dictionary tree.
 * compare_cb is the comparison function, typically strcmp, strcasecmp or
 * irccasecmp.
 */
extern mowgli_dictionary_t *mowgli_dictionary_create(mowgli_dictionary_comparator_func_t compare_cb);

/*
 * mowgli_dictionary_create_named() creates a new dictionary tree which has a name.
 * name is the name, compare_cb is the comparator.
 */
extern mowgli_dictionary_t *mowgli_dictionary_create_named(const char *name, mowgli_dictionary_comparator_func_t compare_cb);

/*
 * mowgli_dictionary_set_comparator_func() resets the comparator used for lookups and
 * insertions in the DTree structure.
 */
extern void mowgli_dictionary_set_comparator_func(mowgli_dictionary_t *dict,
	mowgli_dictionary_comparator_func_t compare_cb);

/*
 * mowgli_dictionary_get_comparator_func() returns the comparator used for lookups and
 * insertions in the DTree structure.
 */
extern mowgli_dictionary_comparator_func_t mowgli_dictionary_get_comparator_func(mowgli_dictionary_t *dict);

/*
 * mowgli_dictionary_get_linear_index() returns the linear index of an object in the
 * DTree structure.
 */
extern int mowgli_dictionary_get_linear_index(mowgli_dictionary_t *dict, const char *key);

/*
 * mowgli_dictionary_destroy() destroys all entries in a dtree, and also optionally calls
 * a defined callback function to destroy any data attached to it.
 */
extern void mowgli_dictionary_destroy(mowgli_dictionary_t *dtree,
	void (*destroy_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
	void *privdata);

/*
 * mowgli_dictionary_foreach() iterates all entries in a dtree, and also optionally calls
 * a defined callback function to use any data attached to it.
 *
 * To shortcircuit iteration, return non-zero from the callback function.
 */
extern void mowgli_dictionary_foreach(mowgli_dictionary_t *dtree,
	int (*foreach_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
	void *privdata);

/*
 * mowgli_dictionary_search() iterates all entries in a dtree, and also optionally calls
 * a defined callback function to use any data attached to it.
 *
 * When the object is found, a non-NULL is returned from the callback, which results
 * in that object being returned to the user.
 */
extern void *mowgli_dictionary_search(mowgli_dictionary_t *dtree,
	void *(*foreach_cb)(mowgli_dictionary_elem_t *delem, void *privdata),
	void *privdata);

/*
 * mowgli_dictionary_foreach_start() begins an iteration over all items
 * keeping state in the given struct. If there is only one iteration
 * in progress at a time, it is permitted to remove the current element
 * of the iteration (but not any other element).
 */
extern void mowgli_dictionary_foreach_start(mowgli_dictionary_t *dtree,
	mowgli_dictionary_iteration_state_t *state);

/*
 * mowgli_dictionary_foreach_cur() returns the current element of the iteration,
 * or NULL if there are no more elements.
 */
extern void *mowgli_dictionary_foreach_cur(mowgli_dictionary_t *dtree,
	mowgli_dictionary_iteration_state_t *state);

/*
 * mowgli_dictionary_foreach_next() moves to the next element.
 */
extern void mowgli_dictionary_foreach_next(mowgli_dictionary_t *dtree,
	mowgli_dictionary_iteration_state_t *state);

/*
 * mowgli_dictionary_add() adds a key->value entry to the dictionary tree.
 */
extern mowgli_dictionary_elem_t *mowgli_dictionary_add(mowgli_dictionary_t *dtree, const char *key, void *data);

/*
 * mowgli_dictionary_find() returns a mowgli_dictionary_elem_t container from a dtree for key 'key'.
 */
extern mowgli_dictionary_elem_t *mowgli_dictionary_find(mowgli_dictionary_t *dtree, const char *key);

/*
 * mowgli_dictionary_find() returns data from a dtree for key 'key'.
 */
extern void *mowgli_dictionary_retrieve(mowgli_dictionary_t *dtree, const char *key);

/*
 * mowgli_dictionary_delete() deletes a key->value entry from the dictionary tree.
 */
extern void *mowgli_dictionary_delete(mowgli_dictionary_t *dtree, const char *key);

void mowgli_dictionary_stats(mowgli_dictionary_t *dict, void (*cb)(const char *line, void *privdata), void *privdata);

#endif
