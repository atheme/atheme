/*
 * atheme-services: A collection of minimalist IRC services
 * sharedheap.c: Shared heaps (mowgli.heap bound to atheme.object)
 *
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
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

typedef struct {
	object_t parent;

	size_t size;
	mowgli_heap_t *heap;

	mowgli_node_t node;
} sharedheap_t;

mowgli_list_t sharedheap_list;

static sharedheap_t *sharedheap_find_by_size(size_t size)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sharedheap_list.head)
	{
		sharedheap_t *s = n->data;

		if (s->size == size)
			return s;
	}

	return NULL;
}

static sharedheap_t *sharedheap_find_by_heap(mowgli_heap_t *heap)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sharedheap_list.head)
	{
		sharedheap_t *s = n->data;

		if (s->heap == heap)
			return s;
	}

	return NULL;
}

static void sharedheap_destroy(sharedheap_t *s)
{
	return_if_fail(s != NULL);

	mowgli_heap_destroy(s->heap);
	mowgli_node_delete(&s->node, &sharedheap_list);

	free(s);
}

static inline size_t sharedheap_prealloc_size(size_t size)
{
	size_t page_size, prealloc_size;

#ifndef MOWGLI_OS_WIN
	page_size = sysconf(_SC_PAGESIZE);
#else
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	page_size = si.dwPageSize;
#endif

	prealloc_size = page_size / size;

#ifdef LARGE_NETWORK
	prealloc_size *= 4;
#endif

	return prealloc_size;
}

static inline size_t sharedheap_normalize_size(size_t size)
{
	size_t normalized;

	normalized = ((size / sizeof(void *)) + ((size / sizeof(void *)) % 2)) * sizeof(void *);

	slog(LG_DEBUG, "sharedheap_normalize_size(%zu): normalized=%zu", size, normalized);

	return normalized;
}

static sharedheap_t *sharedheap_new(size_t size)
{
	sharedheap_t *s;

	s = smalloc(sizeof(sharedheap_t));
	object_init(object(s), NULL, (destructor_t) sharedheap_destroy);

	s->size = size;
	s->heap = mowgli_heap_create(size, sharedheap_prealloc_size(size), BH_NOW);

	mowgli_node_add(s, &s->node, &sharedheap_list);

	return object_sink_ref(s);
}

mowgli_heap_t *sharedheap_get(size_t size)
{
	sharedheap_t *s;

	size = sharedheap_normalize_size(size);

	s = sharedheap_find_by_size(size);

	if (s == NULL)
	{
		if ((s = sharedheap_new(size)) == NULL)
		{
			slog(LG_DEBUG, "sharedheap_get(%zu): mowgli.heap failure", size);
			return NULL;
		}
	}

	soft_assert(s != NULL);

	object_ref(s);

	return s->heap;
}

void sharedheap_unref(mowgli_heap_t *heap)
{
	sharedheap_t *s;

	return_if_fail(heap != NULL);

	s = sharedheap_find_by_heap(heap);

	return_if_fail(s != NULL);

	object_unref(s);
}
