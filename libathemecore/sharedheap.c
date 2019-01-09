/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011-2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * sharedheap.c: Shared heaps (mowgli.heap bound to atheme.object)
 */

#include "atheme.h"

static mowgli_list_t sharedheap_list;

static struct sharedheap *
sharedheap_find_by_size(const size_t size)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sharedheap_list.head)
	{
		struct sharedheap *const s = n->data;

		if (s->size == size)
			return s;
	}

	return NULL;
}

static struct sharedheap *
sharedheap_find_by_heap(mowgli_heap_t *const restrict heap)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sharedheap_list.head)
	{
		struct sharedheap *const s = n->data;

		if (s->heap == heap)
			return s;
	}

	return NULL;
}

static void
sharedheap_destroy(void *const restrict ptr)
{
	return_if_fail(ptr != NULL);

	struct sharedheap *const s = ptr;

	(void) mowgli_node_delete(&s->node, &sharedheap_list);
	(void) mowgli_heap_destroy(s->heap);
	(void) sfree(s);
}

static inline size_t
sharedheap_prealloc_size(const size_t size)
{
#ifndef MOWGLI_OS_WIN
	const size_t page_size = sysconf(_SC_PAGESIZE);
#else
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	const size_t page_size = si.dwPageSize;
#endif

#ifndef LARGE_NETWORK
	const size_t prealloc_size = (page_size / size);
#else
	const size_t prealloc_size = (page_size / size) * 4U;
#endif

	return prealloc_size;
}

static inline size_t
sharedheap_normalize_size(const size_t size)
{
	const size_t normalized = ((size / sizeof(void *)) + ((size / sizeof(void *)) % 2U)) * sizeof(void *);

	(void) slog(LG_DEBUG, "%s: %zu --> %zu", __func__, size, normalized);

	return normalized;
}

static struct sharedheap * ATHEME_FATTR_MALLOC
sharedheap_new(const size_t size)
{
	mowgli_heap_t *const heap = mowgli_heap_create(size, sharedheap_prealloc_size(size), BH_NOW);

	if (! heap)
	{
		(void) slog(LG_DEBUG, "%s: mowgli_heap_create() failed!", __func__);
		return NULL;
	}

	struct sharedheap *const s = smalloc(sizeof *s);

	(void) atheme_object_init(atheme_object(s), NULL, &sharedheap_destroy);
	(void) mowgli_node_add(s, &s->node, &sharedheap_list);

	s->size = size;
	s->heap = heap;

	return s;
}

mowgli_heap_t *
sharedheap_get(const size_t size)
{
	const size_t normalized = sharedheap_normalize_size(size);

	struct sharedheap *s = sharedheap_find_by_size(normalized);

	if (s)
		(void) atheme_object_ref(s);
	else if (! (s = sharedheap_new(normalized)))
		return NULL;

	return s->heap;
}

void
sharedheap_unref(mowgli_heap_t *const restrict heap)
{
	return_if_fail(heap != NULL);

	struct sharedheap *const s = sharedheap_find_by_heap(heap);

	return_if_fail(s != NULL);

	(void) atheme_object_unref(s);
}
