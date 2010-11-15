/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_heap.c: Heap allocation.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2005-2006 Theo Julienne <terminal -at- atheme.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
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

/* A block of memory allocated to the allocator */
struct mowgli_block_
{
	mowgli_node_t node;
	
	/* link back to our heap */
	mowgli_heap_t *heap;
	
	/* pointer to the first item */
	void *data;
	
	/* singly linked list of free items */
	void *first_free;

	int num_allocated;
};

/* A pile of blocks */
struct mowgli_heap_
{
	mowgli_node_t node;
	
	unsigned int elem_size;
	unsigned int mowgli_heap_elems;
	unsigned int free_elems;
	
	unsigned int alloc_size;
	
	unsigned int flags;
	
	mowgli_list_t blocks; /* list of non-empty blocks */

	mowgli_allocation_policy_t *allocator;
	mowgli_boolean_t use_mmap;

	mowgli_block_t *empty_block; /* a single entirely free block, or NULL */
};

typedef struct mowgli_heap_elem_header_ mowgli_heap_elem_header_t;

struct mowgli_heap_elem_header_
{
	union
	{
		mowgli_block_t *block; /* for allocated elems: block ptr */
		mowgli_heap_elem_header_t *next; /* for free elems: next free */
	} un;
};

/* expands a mowgli_heap_t by 1 block */
static void
mowgli_heap_expand(mowgli_heap_t *bh)
{
	mowgli_block_t *block = NULL;
	void *blp = NULL;
	mowgli_heap_elem_header_t *node, *prev;
	char *offset;
	unsigned int a;

	size_t blp_size = sizeof(mowgli_block_t) + (bh->alloc_size * bh->mowgli_heap_elems);

	return_if_fail(bh->empty_block == NULL);
	
#if defined(HAVE_MMAP) && defined(MAP_ANON)
	if (bh->use_mmap)
		blp = mmap(NULL, sizeof(mowgli_block_t) + (bh->alloc_size * bh->mowgli_heap_elems),
			PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	else
#endif
	{
		if (bh->allocator)
			blp = bh->allocator->allocate(blp_size);
		else
			blp = mowgli_alloc(blp_size);
	}

	block = (mowgli_block_t *)blp;
	
	offset = (char*)blp + sizeof(mowgli_block_t);
	block->data = offset;
	block->heap = bh;

	prev = NULL;

	for (a = 0; a < bh->mowgli_heap_elems; a++)
	{
		node = (mowgli_heap_elem_header_t *)offset;
		node->un.next = prev;
		offset += bh->alloc_size;
		prev = node;
	}
	
	block->first_free = prev;

	bh->empty_block = block;
	bh->free_elems += bh->mowgli_heap_elems;
}

/* shrinks a mowgli_heap_t by 1 block. */
static void
mowgli_heap_shrink(mowgli_heap_t *heap, mowgli_block_t *b)
{
	return_if_fail(b != NULL);

	if (b == heap->empty_block)
		heap->empty_block = NULL;
	else
		mowgli_node_delete(&b->node, &heap->blocks);

#ifdef HAVE_MMAP
	if (heap->use_mmap)
		munmap(b, sizeof(mowgli_block_t) + (heap->alloc_size * heap->mowgli_heap_elems));
	else
#endif
		heap->allocator->deallocate(b);

	heap->free_elems -= heap->mowgli_heap_elems;
}

/* creates a new mowgli_heap_t */
mowgli_heap_t *
mowgli_heap_create_full(size_t elem_size, size_t mowgli_heap_elems, unsigned int flags,
	mowgli_allocation_policy_t *allocator)
{
	mowgli_heap_t *bh = mowgli_alloc(sizeof(mowgli_heap_t));
	int numpages, pagesize;

	bh->elem_size = elem_size;
	bh->mowgli_heap_elems = mowgli_heap_elems;
	/* at least 2, this avoids some silly special cases */
	if (bh->mowgli_heap_elems < 2)
		bh->mowgli_heap_elems = 2;
	bh->free_elems = 0;
	
	bh->alloc_size = bh->elem_size + sizeof(mowgli_heap_elem_header_t);

	/* don't waste part of a page */
	if (allocator == NULL)
	{
#ifdef HAVE_MMAP
		pagesize = getpagesize();
#else
		pagesize = 4096;
#endif
		numpages = (sizeof(mowgli_block_t) + (bh->alloc_size * bh->mowgli_heap_elems) + pagesize - 1) / pagesize;
		bh->mowgli_heap_elems = (numpages * pagesize - sizeof(mowgli_block_t)) / bh->alloc_size;
	}
	
	bh->flags = flags;

	bh->allocator = allocator ? allocator : mowgli_allocator_malloc;

#ifdef HAVE_MMAP
	bh->use_mmap = allocator != NULL ? FALSE : TRUE;
#endif
	
	if (flags & BH_NOW)
		mowgli_heap_expand(bh);
	
	return bh;
}

mowgli_heap_t *
mowgli_heap_create(size_t elem_size, size_t mowgli_heap_elems, unsigned int flags)
{
	return mowgli_heap_create_full(elem_size, mowgli_heap_elems, flags, NULL);
}

/* completely frees a mowgli_heap_t and all blocks */
void
mowgli_heap_destroy(mowgli_heap_t *heap)
{
	mowgli_node_t *n, *tn;
	
	MOWGLI_LIST_FOREACH_SAFE(n, tn, heap->blocks.head)
	{
		mowgli_heap_shrink(heap, n->data);
	}
	if (heap->empty_block)
		mowgli_heap_shrink(heap, heap->empty_block);

	/* everything related to heap has gone, time for itself */
	mowgli_free(heap);
}

/* allocates a new item from a mowgli_heap_t */
void *
mowgli_heap_alloc(mowgli_heap_t *heap)
{
	mowgli_node_t *n;
	mowgli_block_t *b;
	mowgli_heap_elem_header_t *h;

	/* no free space? */
	if (heap->free_elems == 0)
	{
		mowgli_heap_expand(heap);

		return_val_if_fail(heap->free_elems != 0, NULL);
	}

	/* try a partially used block before using a fully free block */
	n = heap->blocks.head;
	b = n != NULL ? n->data : NULL;
	if (b == NULL || b->first_free == NULL)
		b = heap->empty_block;
	/* due to above check */
	return_val_if_fail(b != NULL, NULL);

	/* pull the first free node from the list */
	h = b->first_free;
	return_val_if_fail(h != NULL, NULL);

	/* mark it as used */
	b->first_free = h->un.next;
	h->un.block = b;

	/* keep count */
	heap->free_elems--;
	b->num_allocated++;

	/* move it between the lists if needed */
	/* note that a block has at least two items in it, so these cases
	 * cannot both occur in the same allocation */
	if (b->num_allocated == 1)
	{
		heap->empty_block = NULL;
		mowgli_node_add_head(b, &b->node, &heap->blocks);
	}
	else if (b->first_free == NULL)
	{
		/* move full blocks to the end of the list */
		mowgli_node_delete(&b->node, &heap->blocks);
		mowgli_node_add(b, &b->node, &heap->blocks);
	}

#ifdef HEAP_DEBUG		
	/* debug */
	mowgli_log("mowgli_heap_alloc(heap = @%p) -> %p", heap, fn->data);
#endif
	/* return pointer to it */
	return (char *)h + sizeof(mowgli_heap_elem_header_t);
}

/* frees an item back to the mowgli_heap_t */
void
mowgli_heap_free(mowgli_heap_t *heap, void *data)
{
	mowgli_block_t *b;
	mowgli_heap_elem_header_t *h;

	h = (mowgli_heap_elem_header_t *)((char *)data - sizeof(mowgli_heap_elem_header_t));
	b = h->un.block;

	return_if_fail(b->heap == heap);
	return_if_fail(b->num_allocated > 0);

	/* memset the element before returning it to the heap. */
	memset(data, 0, b->heap->elem_size);

	/* mark it as free */
	h->un.next = b->first_free;
	b->first_free = h;

	/* keep count */
	heap->free_elems++;
	b->num_allocated--;
#ifdef HEAP_DEBUG
	/* debug */
	mowgli_log("mowgli_heap_free(heap = @%p, data = %p)", heap, data);
#endif
	/* move it between the lists if needed */
	if (b->num_allocated == 0)
	{
		if (heap->empty_block != NULL)
			mowgli_heap_shrink(heap, heap->empty_block);
		mowgli_node_delete(&b->node, &heap->blocks);
		heap->empty_block = b;
	}
	else if (b->num_allocated == heap->mowgli_heap_elems - 1)
	{
		mowgli_node_delete(&b->node, &heap->blocks);
		mowgli_node_add_head(b, &b->node, &heap->blocks);
	}
}
