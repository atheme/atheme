/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Callback functions. (Inspired from many sources)
 *
 * $Id: callback.c 1895 2005-08-28 18:11:25Z nenolod $
 */

#include "atheme.h"

list_t callback_list;
static BlockHeap *callback_heap;

void callback_init(void)
{
	callback_heap = BlockHeapCreate(sizeof(callback_t), 256);

	if (!callback_heap)
	{
		slog(LG_INFO, "callback_init(): block allocator failure.");
		exit(EXIT_FAILURE);
	}
}

/*
 * callback_register()
 *
 * Inputs:
 *       name, optional initial function
 *
 * Outputs:
 *       callback object
 *
 * Side Effects:
 *       a callback listener is set up
 */
callback_t *callback_register(const char *name,
	void *(*func)(va_list args))
{
	callback_t *cb;

	if (!name)
		return;

	if ((cb = callback_find(name)))
	{
		node_add(func, node_create(), &cb->hooks);
		return cb;
	}

	cb = BlockHeapAlloc(callback_heap);
	if (func)
		node_add(func, node_create(), &cb->hooks);

	if (name)
	{
		strlcpy(cb->name, name, BUFSIZE);
		node_add(cb, &cb->node, &callback_list);
	}

	return cb;
}

/*
 * callback_find()
 *
 * Inputs:
 *       callback name
 *
 * Outputs:
 *       the callback, or NULL
 *
 * Side Effects:
 *       none
 */
callback_t *callback_find(const char *name)
{
	callback_t *cb;
	node_t *n;

	LIST_FOREACH(n, callback_list.head)
	{
		cb = n->data;

		if (!irccmp(cb->name, name))
			return cb;
	}

	return NULL;
}

/*
 * callback_execute()
 *
 * Inputs:
 *       name of callback to execute, args
 *
 * Outputs:
 *       callback return value
 *
 * Side Effects:
 *       a callback is executed
 */
void *callback_execute(const char *name, ...)
{
	callback_t *cb = callback_find(name);
	node_t *n;
	void *(*func)(va_list args);
	typedef void *(*func_t)(va_list args);
	void *res;
	va_list av;

	if (!cb)
		return NULL;

	va_start(av, name);

	LIST_FOREACH(n, cb->hooks.head)
	{
		func = (func_t) n->data;
		res = func(av);
	}

	va_end(av);
	return res;
}

/*
 * callback_inject()
 *
 * Inputs:
 *       callback name, callback function
 *
 * Outputs:
 *       nothing
 *
 * Side Effects:
 *       none
 */
void callback_inject(const char *name,
	void *(*func)(va_list args))
{
	callback_t *cb = callback_find(name);

	if (!cb || !func)
		return;

	node_add(func, node_create(), &cb->hooks);
}

/*
 * callback_remove()
 *
 * Inputs:
 *       callback name, callback function
 *
 * Outputs:
 *       nothing
 *
 * Side Effects:
 *       none
 */
void callback_remove(const char *name,
	void *(*func)(va_list args))
{
	callback_t *cb = callback_find(name);
	node_t *n;

	if (!cb || !func)
		return;

	n = node_find(&cb->hooks, (void *)func);

	if (!n)
		return;

	node_del(n, &cb->hooks);
	node_free(n);
}
