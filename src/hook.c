/*
 * atheme-services: A collection of minimalist IRC services   
 * hook.c: Hook system.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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
		slog(LG_INFO, "hooks_init(): block allocator failed.");
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

hook_t *hook_add_event(const char *name)
{
	hook_t *nh;
	node_t *n;

	if(find_hook(name))
		return;

	nh = new_hook(name);
	n = node_create();

	node_add(nh, n, &hooks);

	return nh;
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
		h = hook_add_event(event);

	n = node_create();

	node_add((void *) handler, n, &h->hooks);
}

void hook_add_hook_first(const char *event, void (*handler)(void *data))
{
	hook_t *h;
	node_t *n;

	if (!(h = find_hook(event)))
		h = hook_add_event(event);

	n = node_create();

	node_add_head((void *) handler, n, &h->hooks);
}

void hook_call_event(const char *event, void *dptr)
{
	hook_t *h;
	node_t *n, *tn;
	void (*func)(void *data);

	if (!(h = find_hook(event)))
		return;

	LIST_FOREACH_SAFE(n, tn, h->hooks.head)
	{
		func = (void (*)(void *)) n->data;
		func(dptr);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
