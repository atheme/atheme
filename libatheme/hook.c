/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system. Idea taken from hybrid.
 *
 * $Id: hook.c 3053 2005-10-20 18:04:13Z nenolod $
 */

#include <org.atheme.claro.base>

list_t hooks;
static BlockHeap *hook_heap;
static hook_t *find_hook(const char *name);

void hooks_init()
{
	hook_heap = BlockHeapCreate(sizeof(hook_t), 1024);

	if (!hook_heap)
	{
		clog(LG_INFO, "hooks_init(): block allocator failed.");
		exit(EXIT_SUCCESS);
	}
}

static hook_t *new_hook(const char *name)
{
	hook_t *h;

	h = BlockHeapAlloc(hook_heap);
	h->name = sstrdup(name);

	return h;
}

void hook_add_event(const char *name)
{
	hook_t *nh;
	node_t *n;

	if(find_hook(name))
		return;

	nh = new_hook(name);
	n = node_create();

	node_add(nh, n, &hooks);
}

void hook_del_event(const char *name)
{
	node_t *n;
	hook_t *h;

	LIST_FOREACH(n, hooks.head)
	{
		h = n->data;

		if (!strcmp(h->name, name))
		{
			node_del(n, &hooks);
			BlockHeapFree(hook_heap, h);
			return;
		}
	}
}

static hook_t *find_hook(const char *name)
{
	node_t *n;
	hook_t *h;

	LIST_FOREACH(n, hooks.head)
	{
		h = n->data;

		if (!strcmp(h->name, name))
			return h;
	}

	return NULL;
}

void hook_del_hook(const char *event, void (*handler)(void *data))
{
	node_t *n, *n2;
	hook_t *h;

	if (!(h = find_hook(event)))
		return;

	LIST_FOREACH_SAFE(n, n2, h->hooks.head)
	{
		if (handler == (void (*)(void *)) n->data)
			node_del(n, &h->hooks);
	}
}

void hook_add_hook(const char *event, void (*handler)(void *data))
{
	hook_t *h;
	node_t *n;

	if (!(h = find_hook(event)))
		return;

	n = node_create();

	node_add((void *) handler, n, &h->hooks);
}

void hook_call_event(const char *event, void *dptr)
{
	hook_t *h;
	node_t *n;
	void (*func)(void *data);

	if (!(h = find_hook(event)))
		return;

	LIST_FOREACH(n, h->hooks.head)
	{
		func = (void (*)(void *)) n->data;
		func(dptr);
	}
}

#ifdef NOTYET
void hook_exec_event(const char *event, ...)
{
	hook_t *h;
	node_t *n;
	void (*func)(void *data);
	va_list ap;

	if (!(h = find_hook(event)))
		return;

	LIST_FOREACH(n, h->hooks.head)
	{
		func = (void (*)(void *)) n->data;
		va_start(ap, event);
		func(&ap);
		va_end(ap);
	}
}
#endif
