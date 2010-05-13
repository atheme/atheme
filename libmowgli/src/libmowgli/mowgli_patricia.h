/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_patricia.h: Dictionary-based storage.
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

#ifndef __MOWGLI_PATRICIA_H__
#define __MOWGLI_PATRICIA_H__

struct mowgli_patricia_; /* defined in src/patricia.c */
struct mowgli_patricia_elem_; /* defined in src/patricia.c */

typedef struct mowgli_patricia_ mowgli_patricia_t;
typedef struct mowgli_patricia_elem_ mowgli_patricia_elem_t;

/*
 * mowgli_patricia_iteration_state_t, private.
 */
struct mowgli_patricia_iteration_state_
{
	mowgli_patricia_elem_t *cur, *next;
	void *pspare[4];
	int ispare[4];
};

typedef struct mowgli_patricia_iteration_state_ mowgli_patricia_iteration_state_t;

/*
 * this is a convenience macro for inlining iteration of dictionaries.
 */
#define MOWGLI_PATRICIA_FOREACH(element, state, dict) for (mowgli_patricia_foreach_start((dict), (state)); (element = mowgli_patricia_foreach_cur((dict), (state))); mowgli_patricia_foreach_next((dict), (state)))

/*
 * mowgli_patricia_create() creates a new patricia tree of the defined resolution.
 * compare_cb is the canonizing function.
 */
extern mowgli_patricia_t *mowgli_patricia_create(void (*canonize_cb)(char *key));

/*
 * mowgli_patricia_destroy() destroys all entries in a dtree, and also optionally calls
 * a defined callback function to destroy any data attached to it.
 */
extern void mowgli_patricia_destroy(mowgli_patricia_t *dtree,
	void (*destroy_cb)(const char *key, void *data, void *privdata),
	void *privdata);

/*
 * mowgli_patricia_foreach() iterates all entries in a dtree, and also optionally calls
 * a defined callback function to use any data attached to it.
 *
 * To shortcircuit iteration, return non-zero from the callback function.
 */
extern void mowgli_patricia_foreach(mowgli_patricia_t *dtree,
	int (*foreach_cb)(const char *key, void *data, void *privdata),
	void *privdata);

/*
 * mowgli_patricia_search() iterates all entries in a dtree, and also optionally calls
 * a defined callback function to use any data attached to it.
 *
 * When the object is found, a non-NULL is returned from the callback, which results
 * in that object being returned to the user.
 */
extern void *mowgli_patricia_search(mowgli_patricia_t *dtree,
	void *(*foreach_cb)(const char *key, void *data, void *privdata),
	void *privdata);

/*
 * mowgli_patricia_foreach_start() begins an iteration over all items
 * keeping state in the given struct. If there is only one iteration
 * in progress at a time, it is permitted to remove the current element
 * of the iteration (but not any other element).
 */
extern void mowgli_patricia_foreach_start(mowgli_patricia_t *dtree,
	mowgli_patricia_iteration_state_t *state);

/*
 * mowgli_patricia_foreach_cur() returns the current element of the iteration,
 * or NULL if there are no more elements.
 */
extern void *mowgli_patricia_foreach_cur(mowgli_patricia_t *dtree,
	mowgli_patricia_iteration_state_t *state);

/*
 * mowgli_patricia_foreach_next() moves to the next element.
 */
extern void mowgli_patricia_foreach_next(mowgli_patricia_t *dtree,
	mowgli_patricia_iteration_state_t *state);

/*
 * mowgli_patricia_add() adds a key->value entry to the patricia tree.
 */
extern mowgli_boolean_t mowgli_patricia_add(mowgli_patricia_t *dtree, const char *key, void *data);

/*
 * mowgli_patricia_find() returns data from a dtree for key 'key'.
 */
extern void *mowgli_patricia_retrieve(mowgli_patricia_t *dtree, const char *key);

/*
 * mowgli_patricia_delete() deletes a key->value entry from the patricia tree.
 */
extern void *mowgli_patricia_delete(mowgli_patricia_t *dtree, const char *key);

unsigned int mowgli_patricia_size(mowgli_patricia_t *dict);
void mowgli_patricia_stats(mowgli_patricia_t *dict, void (*cb)(const char *line, void *privdata), void *privdata);

#endif
