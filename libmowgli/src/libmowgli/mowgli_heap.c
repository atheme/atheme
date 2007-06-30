/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_heap.c: Heap allocation.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2005-2006 Theo Julienne <terminal -at- atheme.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * Legal note: code devised from claro.base.block module r288 (Pre MPL)
 */

#include "mowgli.h"

#ifdef HAVE_MMAP
# include <sys/mman.h>
# if !defined(MAP_ANON) && defined(MAP_ANONYMOUS)
#  define MAP_ANON MAP_ANONYMOUS
# endif
#endif

/* expands a mowgli_heap_t by 1 block */
static void mowgli_heap_expand(mowgli_heap_t *bh)
{
	mowgli_block_t *block;
	void *blp;
	mowgli_node_t *node;
	void *offset;
	int a;
	
#if defined(HAVE_MMAP) && defined(MAP_ANON)
	blp = mmap(NULL, sizeof(mowgli_block_t) + (bh->alloc_size * bh->mowgli_heap_elems),
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#else
	blp = mowgli_alloc(sizeof(mowgli_block_t) + (bh->alloc_size * bh->mowgli_heap_elems));
#endif
	block = (mowgli_block_t *)blp;
	
	offset = blp + sizeof(mowgli_block_t);
	block->data = offset;
	block->heap = bh;

	for (a = 0; a < bh->mowgli_heap_elems; a++)
	{
		node = (mowgli_node_t *)offset;
		node->prev = node->next = NULL;

		mowgli_node_add(offset + sizeof(mowgli_node_t), node, &block->free_list);

		offset += bh->alloc_size;
	}
	
	mowgli_node_add(block, &block->node, &bh->blocks);
	
	bh->free_elems += bh->mowgli_heap_elems;
}

/* shrinks a mowgli_heap_t by 1 block. */
static void mowgli_heap_shrink(mowgli_block_t *b)
{
	mowgli_heap_t *heap;

	return_if_fail(b != NULL);
	return_if_fail(b->heap != NULL);
	return_if_fail(MOWGLI_LIST_LENGTH(&b->used_list) == 0);

	heap = b->heap;

	mowgli_node_delete(&b->node, &heap->blocks);

#ifdef HAVE_MMAP
	munmap(b, sizeof(mowgli_block_t) + (heap->alloc_size * heap->mowgli_heap_elems));
#else
	mowgli_free(b);
#endif

	heap->free_elems -= heap->mowgli_heap_elems;
}

/* creates a new mowgli_heap_t */
mowgli_heap_t *mowgli_heap_create(size_t elem_size, size_t mowgli_heap_elems, unsigned int flags)
{
	mowgli_heap_t *bh = mowgli_alloc(sizeof(mowgli_heap_t));
	
	bh->elem_size = elem_size;
	bh->mowgli_heap_elems = mowgli_heap_elems;
	bh->free_elems = 0;
	
	bh->alloc_size = bh->elem_size + sizeof(mowgli_node_t);
	
	bh->flags = flags;
	
	if (flags & BH_NOW)
		mowgli_heap_expand(bh);
	
	return bh;
}

/* completely frees a mowgli_heap_t and all blocks */
void mowgli_heap_destroy(mowgli_heap_t *heap)
{
	mowgli_node_t *n, *tn;
	
	MOWGLI_LIST_FOREACH_SAFE(n, tn, heap->blocks.head)
	{
		mowgli_node_delete(n, &heap->blocks); 
		mowgli_free(n);
	}

	/* everything related to heap has gone, time for itself */
	mowgli_free(heap);
}

/* allocates a new item from a mowgli_heap_t */
void *mowgli_heap_alloc(mowgli_heap_t *heap)
{
	mowgli_node_t *n, *fn;
	mowgli_block_t *b;

	/* no free space? */
	if (heap->free_elems == 0)
	{
		mowgli_heap_expand(heap);

		return_val_if_fail(heap->free_elems != 0, NULL);
	}

	MOWGLI_LIST_FOREACH(n, heap->blocks.head)
	{
		b = (mowgli_block_t *) n->data;
		
		if (MOWGLI_LIST_LENGTH(&b->free_list) == 0)
			continue;
		
		/* pull the first free node from the list */
		fn = b->free_list.head;
		
		/* mark it as used */
		mowgli_node_move(fn, &b->free_list, &b->used_list);
		
		/* keep count */
		heap->free_elems--;

#ifdef HEAP_DEBUG		
		/* debug */
		mowgli_log("mowgli_heap_alloc(heap = @%p) -> %p", heap, fn->data);
#endif
		/* return pointer to it */
		return fn->data;
	}
	
	/* this should never happen */
	mowgli_throw_exception_fatal(mowgli.heap.internal_error_exception);
	
	return NULL;
}

/* frees an item back to the mowgli_heap_t */
void mowgli_heap_free(mowgli_heap_t *heap, void *data)
{
	mowgli_node_t *n, *tn, *dn;
	mowgli_block_t *b;
	
	MOWGLI_LIST_FOREACH_SAFE(n, tn, heap->blocks.head)
	{
		b = (mowgli_block_t *) n->data;

		/* see if the element belongs to this list */
		dn = mowgli_node_find(data, &b->used_list);

		if (dn != NULL)
		{
			/* mark it as free */
			mowgli_node_move(dn, &b->used_list, &b->free_list);

			/* keep count */
			heap->free_elems++;
#ifdef HEAP_DEBUG
			/* debug */
			mowgli_log("mowgli_heap_free(heap = @%p, data = %p)", heap, data);
#endif
			/* if this block is entirely unfree, free it. */
			if (MOWGLI_LIST_LENGTH(&b->used_list) == 0 && MOWGLI_LIST_LENGTH(&heap->blocks) > 2)
				mowgli_heap_shrink(b);

			/* we're done */
			return;
		}
		
		/* and just in case, check it's not already free'd */
		dn = mowgli_node_find(data, &b->free_list);
		
		if (dn != NULL)
			mowgli_throw_exception(mowgli.heap.block_already_free_exception);
	}
	
	/* this should never happen */
	mowgli_throw_exception_fatal(mowgli.heap.invalid_pointer_exception);
}
