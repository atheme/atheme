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

mowgli_patricia_t *hooks;
mowgli_heap_t *hook_heap;
static hook_t *find_hook(const char *name);

void hooks_init()
{
	hooks = mowgli_patricia_create(strcasecanon);
	hook_heap = mowgli_heap_create(sizeof(hook_t), 1024, BH_NOW);

	if (!hook_heap || !hooks)
	{
		slog(LG_INFO, "hooks_init(): block allocator failed.");
		exit(EXIT_SUCCESS);
	}
}

hook_t *hook_add_event(const char *name)
{
	hook_t *nh;

	if((nh = find_hook(name)) != NULL)
		return nh;

	nh = mowgli_heap_alloc(hook_heap);
	nh->name = sstrdup(name);

	mowgli_patricia_add(hooks, name, nh);

	return nh;
}

void hook_del_event(const char *name)
{
	hook_t *h;

	if ((h = find_hook(name)) != NULL)
	{
		mowgli_node_t *n, *tn;

		MOWGLI_ITER_FOREACH_SAFE(n, tn, h->hooks.head)
		{
			mowgli_node_delete(n, &h->hooks);
			mowgli_node_free(n);
		}

		mowgli_heap_free(hook_heap, h);
		return;
	}
}

static hook_t *find_hook(const char *name)
{
	return mowgli_patricia_retrieve(hooks, name);
}

void hook_del_hook(const char *event, void (*handler)(void *data))
{
	mowgli_node_t *n, *n2;
	hook_t *h;

	if (!(h = find_hook(event)))
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, n2, h->hooks.head)
	{
		if (handler == (void (*)(void *)) n->data)
		{
			mowgli_node_delete(n, &h->hooks);
			mowgli_node_free(n);
		}
	}
}

void hook_add_hook(const char *event, void (*handler)(void *data))
{
	hook_t *h;
	mowgli_node_t *n;

	if (!(h = find_hook(event)))
		h = hook_add_event(event);

	n = mowgli_node_create();
	mowgli_node_add((void *) handler, n, &h->hooks);
}

void hook_add_hook_first(const char *event, void (*handler)(void *data))
{
	hook_t *h;
	mowgli_node_t *n;

	if (!(h = find_hook(event)))
		h = hook_add_event(event);

	n = mowgli_node_create();
	mowgli_node_add_head((void *) handler, n, &h->hooks);
}

void hook_call_event(const char *event, void *dptr)
{
	hook_t *h;
	mowgli_node_t *n, *tn;
	void (*func)(void *data);

	if (!(h = find_hook(event)))
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, h->hooks.head)
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
