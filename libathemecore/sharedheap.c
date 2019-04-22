/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2011-2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * sharedheap.c: Shared heaps (mowgli.heap bound to atheme.object)
 */

#include <atheme.h>
#include "internal.h"

#ifdef ATHEME_ENABLE_HEAP_ALLOCATOR

static mowgli_list_t sharedheap_list;

static struct sharedheap *
sharedheap_find_by_size(const size_t size)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sharedheap_list.head)
	{
		struct sharedheap *const s = n->data;

		if (s->size == size)
		{
#ifdef HEAP_DEBUG
			(void) slog(LG_DEBUG, "%s: %zu --> %p", MOWGLI_FUNC_NAME, size, (void *) s);
#endif
			return s;
		}
	}

#ifdef HEAP_DEBUG
	(void) slog(LG_DEBUG, "%s: %zu --> NULL", MOWGLI_FUNC_NAME, size);
#endif

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
		{
#ifdef HEAP_DEBUG
			(void) slog(LG_DEBUG, "%s: %p --> %p", MOWGLI_FUNC_NAME, (void *) heap, (void *) s);
#endif
			return s;
		}
	}

#ifdef HEAP_DEBUG
	(void) slog(LG_DEBUG, "%s: %p --> NULL", MOWGLI_FUNC_NAME, (void *) heap);
#endif

	return NULL;
}

static void
sharedheap_destroy(void *const restrict ptr)
{
	return_if_fail(ptr != NULL);

	struct sharedheap *const s = ptr;

	const size_t elem_size = s->size;

	(void) mowgli_node_delete(&s->node, &sharedheap_list);
	(void) mowgli_heap_destroy(s->heap);
	(void) sfree(s);

#ifdef HEAP_DEBUG
	(void) slog(LG_DEBUG, "%s: sharedheap@%p: destroyed (elem_size %zu)", MOWGLI_FUNC_NAME, ptr, elem_size);
#endif
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

#ifdef ATHEME_ENABLE_LARGE_NET
	const size_t prealloc_size = (page_size / size) * 4U;
#else
	const size_t prealloc_size = (page_size / size);
#endif

#ifdef HEAP_DEBUG
	(void) slog(LG_DEBUG, "%s: %zu --> %zu", MOWGLI_FUNC_NAME, size, prealloc_size);
#endif

	return prealloc_size;
}

static inline size_t
sharedheap_normalize_size(const size_t size)
{
	const size_t normalized = ((size / sizeof(void *)) + ((size / sizeof(void *)) % 2U)) * sizeof(void *);

#ifdef HEAP_DEBUG
	(void) slog(LG_DEBUG, "%s: %zu --> %zu", MOWGLI_FUNC_NAME, size, normalized);
#endif

	return normalized;
}

static struct sharedheap * ATHEME_FATTR_MALLOC
sharedheap_new(const size_t size)
{
	mowgli_heap_t *const heap = mowgli_heap_create(size, sharedheap_prealloc_size(size), BH_NOW);

	if (! heap)
	{
		(void) slog(LG_ERROR, "%s: mowgli_heap_create() failed!", MOWGLI_FUNC_NAME);
		return NULL;
	}

	struct sharedheap *const s = smalloc(sizeof *s);

	(void) atheme_object_init(atheme_object(s), NULL, &sharedheap_destroy);
	(void) mowgli_node_add(s, &s->node, &sharedheap_list);

	s->size = size;
	s->heap = heap;

#ifdef HEAP_DEBUG
	(void) slog(LG_DEBUG, "%s: created (elem_size %zu)", MOWGLI_FUNC_NAME, size);
#endif

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

#else /* ATHEME_ENABLE_HEAP_ALLOCATOR */

mowgli_heap_t *
sharedheap_get(const size_t size)
{
	mowgli_heap_t *const heap = mowgli_heap_create(size, 2, BH_NOW);

	if (! heap)
		return NULL;

	return heap;
}

void
sharedheap_unref(mowgli_heap_t *const restrict heap)
{
	return_if_fail(heap != NULL);

	(void) mowgli_heap_destroy(heap);
}

#endif /* !ATHEME_ENABLE_HEAP_ALLOCATOR */
