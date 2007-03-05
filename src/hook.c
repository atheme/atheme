/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * A hook system. Idea taken from hybrid.
 *
 * $Id: hook.c 7823 2007-03-05 23:20:25Z pippijn $
 */

#include "atheme.h"
#include "internal.h"

list_t hooks;
static BlockHeap *hook_heap;
static hook_t *find_hook(const char *name);

void hooks_init()
{
	hook_heap = BlockHeapCreate(sizeof(hook_t), 1024);

	if (!hook_heap)
	{
		slog(LG_INFO, gettext("hooks_init(): block allocator failed."));
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
		{
			node_del(n, &h->hooks);
			node_free(n);
		}
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

void hook_add_hook_first(const char *event, void (*handler)(void *data))
{
	hook_t *h;
	node_t *n;

	if (!(h = find_hook(event)))
		return;

	n = node_create();

	node_add_head((void *) handler, n, &h->hooks);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
