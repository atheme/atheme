/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Shared heaps (mowgli.heap bound to atheme.object)
 */

#ifndef ATHEME_INC_SHAREDHEAP_H
#define ATHEME_INC_SHAREDHEAP_H 1

#include <atheme/object.h>
#include <atheme/stdheaders.h>

struct sharedheap
{
	struct atheme_object    parent;
	mowgli_node_t           node;
	mowgli_heap_t *         heap;
	size_t                  size;
};

mowgli_heap_t *sharedheap_get(size_t size);
void sharedheap_unref(mowgli_heap_t *heap);

#endif /* !ATHEME_INC_SHAREDHEAP_H */
