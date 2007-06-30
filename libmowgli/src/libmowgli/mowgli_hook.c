/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_hook.c: Hooks.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007 Giacomo Lozito <james -at- develia.org>
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

static mowgli_dictionary_t *mowgli_hooks = NULL;
static mowgli_heap_t *mowgli_hook_item_heap;

void
mowgli_hook_init(void)
{
	mowgli_hooks = mowgli_dictionary_create(32, strcasecmp);
	mowgli_hook_item_heap = mowgli_heap_create(sizeof(mowgli_hook_item_t), 64, BH_NOW);
}

static mowgli_hook_t *
mowgli_hook_find(const char *name)
{
	return mowgli_dictionary_retrieve(mowgli_hooks, name);
}

void
mowgli_hook_register(const char *name)
{
	mowgli_hook_t *hook;

	return_if_fail(name != NULL);
	return_if_fail((hook = mowgli_hook_find(name)) == NULL);

	hook = mowgli_alloc(sizeof(mowgli_hook_t));
	hook->name = strdup(name);

	mowgli_dictionary_add(mowgli_hooks, hook->name, hook);
}

int
mowgli_hook_associate(const char *name, mowgli_hook_function_t func, void *user_data)
{
	mowgli_hook_t *hook;
	mowgli_hook_item_t *hookitem;

	return_val_if_fail(name != NULL, -1);
	return_val_if_fail(func != NULL, -1);

	hook = mowgli_hook_find(name);

	if (hook == NULL)
	{
		mowgli_hook_register(name);
		hook = mowgli_hook_find(name);
	}

	/* this *cant* happen */
	return_val_if_fail(hook != NULL, -1);

	hookitem = mowgli_heap_alloc(mowgli_hook_item_heap);
	hookitem->func = func;
	hookitem->user_data = user_data;

	mowgli_node_add(hookitem, &hookitem->node, &hook->items);

	return 0;
}

int
mowgli_hook_dissociate(const char *name, mowgli_hook_function_t func)
{
	mowgli_hook_t *hook;
	mowgli_node_t *n, *tn;

	return_val_if_fail(name != NULL, -1);
	return_val_if_fail(func != NULL, -1);

	hook = mowgli_hook_find(name);

	if (hook == NULL)
		return -1;

	MOWGLI_LIST_FOREACH_SAFE(n, tn, hook->items.head)
	{
		mowgli_hook_item_t *hookitem = n->data;

		if (hookitem->func == func)
		{
			mowgli_node_delete(&hookitem->node, &hook->items);
			mowgli_heap_free(mowgli_hook_item_heap, hookitem);

			return 0;
	        }
	}

	return -1;
}

void
mowgli_hook_call(const char *name, void *hook_data)
{
	mowgli_hook_t *hook;
	mowgli_node_t *n;

	return_if_fail(name != NULL);

	hook = mowgli_hook_find(name);

	if (hook == NULL)
		return;

	MOWGLI_LIST_FOREACH(n, hook->items.head)
	{
		mowgli_hook_item_t *hookitem = n->data;

		return_if_fail(hookitem->func != NULL);

		hookitem->func(hook_data, hookitem->user_data);
	}
}
