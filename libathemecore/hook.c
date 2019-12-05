/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * hook.c: Hook system.
 */

#include <atheme.h>
#include "internal.h"

static mowgli_patricia_t *hooks = NULL;
static mowgli_heap_t *hook_heap = NULL;
static mowgli_heap_t *hook_privfn_heap = NULL;

typedef struct {
	struct hook *hook;
	void *dptr;
	mowgli_node_t node;
	unsigned int flags;
} hook_run_ctx_t;

typedef struct {
	hook_fn hookfn;
	mowgli_node_t node;
} hook_privfn_ctx_t;

#define HF_RUN		0x1
#define HF_STOP		0x2

static mowgli_list_t hook_run_stack = { NULL, NULL, 0 };

void
hooks_init(void)
{
	hooks = mowgli_patricia_create(strcasecanon);
	hook_heap = sharedheap_get(sizeof(struct hook));
	hook_privfn_heap = sharedheap_get(sizeof(hook_privfn_ctx_t));

	if (hook_heap == NULL || hook_privfn_heap == NULL || hooks == NULL)
	{
		slog(LG_INFO, "hooks_init(): block allocator failed.");
		exit(EXIT_SUCCESS);
	}
}

static inline struct hook *
hook_find(const char *name)
{
	return mowgli_patricia_retrieve(hooks, name);
}

static struct hook *
hook_add_event(const char *name)
{
	struct hook *nh;

	if((nh = hook_find(name)) != NULL)
		return nh;

	nh = mowgli_heap_alloc(hook_heap);
	nh->name = strshare_get(name);

	mowgli_patricia_add(hooks, nh->name, nh);

	return nh;
}

static inline void
hook_destroy(struct hook *hook, hook_privfn_ctx_t *priv)
{
	mowgli_node_delete(&priv->node, &hook->hooks);
	mowgli_heap_free(hook_privfn_heap, priv);
}

void
hook_del_hook(const char *event, hook_fn handler)
{
	mowgli_node_t *n, *n2;
	struct hook *h;

	return_if_fail(event != NULL);
	return_if_fail(handler != NULL);

	h = hook_find(event);
	if (h == NULL)
		return;

	MOWGLI_ITER_FOREACH_SAFE(n, n2, h->hooks.head)
	{
		hook_privfn_ctx_t *priv = n->data;

		if (handler == priv->hookfn)
			hook_destroy(h, n->data);
	}
}

static inline hook_privfn_ctx_t *
hook_create_and_add(struct hook *hook, hook_fn handler,
	void (*addfn)(void *data, mowgli_node_t *node, mowgli_list_t *list))
{
	hook_privfn_ctx_t *priv;

	return_val_if_fail(hook != NULL, NULL);
	return_val_if_fail(handler != NULL, NULL);
	return_val_if_fail(addfn != NULL, NULL);

	priv = mowgli_heap_alloc(hook_privfn_heap);
	priv->hookfn = handler;

	addfn(priv, &priv->node, &hook->hooks);

	return priv;
}

void
hook_add_hook(const char *event, hook_fn handler)
{
	struct hook *h;
	hook_privfn_ctx_t *priv;

	return_if_fail(event != NULL);
	return_if_fail(handler != NULL);

	h = hook_find(event);
	if (h == NULL)
		h = hook_add_event(event);

	hook_create_and_add(h, handler, mowgli_node_add);
}

void
hook_add_hook_first(const char *event, hook_fn handler)
{
	struct hook *h;

	return_if_fail(event != NULL);
	return_if_fail(handler != NULL);

	h = hook_find(event);
	if (h == NULL)
		h = hook_add_event(event);

	hook_create_and_add(h, handler, mowgli_node_add_head);
}

void
hook_call_event(const char *event, void *dptr)
{
	hook_run_ctx_t ctx;
	mowgli_node_t *n, *tn;
	void (*func)(void *data);

	return_if_fail(event != NULL);

	ctx.hook = hook_find(event);
	if (ctx.hook == NULL)
		return;

	ctx.dptr = dptr;
	ctx.flags = HF_RUN;

	mowgli_node_add_head(&ctx, &ctx.node, &hook_run_stack);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, ctx.hook->hooks.head)
	{
		hook_privfn_ctx_t *priv = n->data;

		priv->hookfn(ctx.dptr);
		if (ctx.flags & HF_STOP)
			goto out;
	}

out:
	mowgli_node_delete(&ctx.node, &hook_run_stack);
}

static inline hook_run_ctx_t *
hook_run_stack_highest(void)
{
	if (hook_run_stack.head == NULL)
		return NULL;

	return hook_run_stack.head->data;
}

void
hook_stop(void)
{
	hook_run_ctx_t *ctx;

	ctx = hook_run_stack_highest();
	if (ctx == NULL)
		return;

	ctx->flags |= HF_STOP;
}

void
hook_continue(void *newptr)
{
	hook_run_ctx_t *ctx;

	ctx = hook_run_stack_highest();
	if (ctx == NULL)
		return;

	ctx->dptr = newptr;
	ctx->flags &= ~HF_STOP;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
