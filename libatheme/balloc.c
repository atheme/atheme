/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the block allocator.
 *
 * $Id: balloc.c 2235 2005-09-14 07:29:13Z nenolod $
 */

#include "atheme.h"

/* please no IOCCC entries --nenolod */
#ifndef _WIN32
#  ifdef HAVE_MMAP		/* We've got mmap() that is good */
#   include <sys/mman.h>
#  ifdef MAP_ANONYMOUS
#   ifndef MAP_ANON
#    define MAP_ANON MAP_ANONYMOUS
#   endif
#  endif
# endif
#else
# undef HAVE_MMAP		/* broken on windows */
#endif

/* We don't have mmap()? That's ok, we have a shim. --nenolod */
#ifndef HAVE_MMAP
# define MAP_PRIVATE 0
# define MAP_ANON 0
# define MAP_FAILED 0
# define PROT_READ 0
# define PROT_WRITE 0
#endif

static boolean_t newblock(heap_t *);
static boolean_t bdispose(heap_t *);
static void block_heap_gc(void *unused);
static list_t heap_lists;

#define blockheap_fail(x) _blockheap_fail(x, __FILE__, __LINE__)

static void _blockheap_fail(const char *reason, const char *file, int line)
{
	slog(LG_INFO, "Blockheap failure: %s (%s:%d)", reason, file, line);
	runflags |= RF_SHUTDOWN;
}

#ifndef HAVE_MMAP
/*
 * static void mmap(void *hint, size_t len, uint32_t prot,
 *                  uint32_t flags, uint16_t fd, uint16_t off)
 *
 * Inputs       - mmap style arguments.
 * Output       - None
 * Side Effects - Memory is allocated to the block allocator.
 */
static void *mmap(void *hint, size_t len, uint32_t prot,
		  uint32_t flags, uint16_t fd, uint16_t off)
{
	void *ptr = smalloc(len);

	if (!ptr)
		blockheap_fail("smalloc() failed.");

	return ptr;
}

/*
 * static void munmap(void *addr, size_t len)
 *
 * Inputs       - munmap style arguments.
 * Output       - None
 * Side Effects - Memory is returned back to the OS.
 */
static void munmap(void *addr, size_t len)
{
	if (!addr)
		blockheap_fail("no address to unmap.");

	free(addr);
}
#endif

/*
 * static void free_block(void *ptr, size_t size)
 *
 * Inputs: The block and its size
 * Output: None
 * Side Effects: Returns memory for the block back to the OS
 */
static void free_block(void *ptr, size_t size)
{
	munmap(ptr, size);
}


/*
 * void initBlockHeap(void)
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: Initializes the block heap
 */

void init_balloc(void)
{
	event_add("block_heap_gc", block_heap_gc, NULL, 60);
}

/*
 * get_block()
 * 
 * Inputs:
 *       Size of block to allocate
 *
 * Output:
 *       Pointer to new block
 *
 * Side Effects:
 *       New memory is mapped (or allocated if your system sucks)
 */
static void *get_block(size_t size)
{
	void *ptr;
	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

	if (ptr == MAP_FAILED)
		ptr = NULL;

	return (ptr);
}

/*
 * block_heap_gc()
 *
 * Inputs:
 *       none
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       walks the heap and performs garbage collection on
 *       the fly
 *
 * Notes:
 *       could be rather effective as a thread
 */
static void block_heap_gc(void *unused)
{
	node_t *ptr, *tptr;

	LIST_FOREACH_SAFE(ptr, tptr, heap_lists.head)
		bdispose(ptr->data);
}

/*
 * newblock()
 *
 * Inputs:
 *       a memory heap to allocate new memory to
 *
 * Outputs:
 *       TRUE if successful, FALSE if not.
 *
 * Side Effects:
 *       Memory is allocated to the heap given to the routine
 */
static boolean_t newblock(heap_t *bh)
{
	chunk_t *newblk;
	block_t *b;
	unsigned long i;
	void *offset;

	/* Setup the initial data structure. */
	b = (block_t *) scalloc(1, sizeof(block_t));

	if (b == NULL)
	{
		slog(LG_DEBUG, "newblock(): b == NULL, failure");
		return FALSE;
	}

	b->free_list.head = b->free_list.tail = NULL;
	b->used_list.head = b->used_list.tail = NULL;

	b->alloc_size = (bh->elements + 1) * (bh->chunk_size + sizeof(chunk_t));

	b->element = get_block(b->alloc_size);

	if (b->element == NULL)
		return FALSE;

	offset = b->element;

	/* Setup our blocks now */
	for (i = 0; i < bh->elements; i++)
	{
		void *data;
		newblk = (chunk_t *) offset;
		data = (void *)((size_t) offset + sizeof(chunk_t));
		newblk->parent = b;
		node_add(data, &newblk->self, &b->free_list);
		offset = (void *)(offset + bh->chunk_size + sizeof(chunk_t));
	}

	++bh->allocated;
	bh->freecount += bh->elements;
	node_add(b, &b->self, &bh->blocks);

	return TRUE;
}

/*
 * bcreate()
 *
 * Inputs:
 *       heap name, element size, amount of elements per block
 *
 * Outputs:
 *       heap object
 *
 * Side Effects:
 *       sets up a block allocator
 */
heap_t *bcreate(char *name, size_t elemsize, int elemsperblock)
{
	heap_t *h;

	/* Catch idiotic requests up front */
	if (!name || (elemsize <= 0) || (elemsperblock <= 0))
		return NULL;

	/* Allocate our new BlockHeap */
	h = (heap_t *)scalloc(1, sizeof(heap_t));
	if (h == NULL)
	{
		slog(LG_INFO, "Attempt to calloc() failed: (%s:%d)", __FILE__, __LINE__);
		runflags |= RF_SHUTDOWN;
	}

	if ((elemsize % sizeof(void *)) != 0)
	{
		/* Pad to even pointer boundary */
		elemsize += sizeof(void *);
		elemsize &= ~(sizeof(void *) - 1);
	}

	h->name = sstrdup(name);
	h->chunk_size = elemsize;
	h->elements = elemsperblock;
	h->allocated = 0;
	h->freecount = 0;

	h->blocks.head = h->blocks.tail = NULL;
	h->blocks.count = 0;

	/* Be sure our malloc was successful */
	if (!newblock(h))
	{
		if (h != NULL)
			free(h);
		slog(LG_INFO, "newblock() failed");
		runflags |= RF_SHUTDOWN;
	}

	if (h == NULL)
		blockheap_fail("h == NULL when it shouldn't be");

	node_add(h, &h->hlist, &heap_lists);
	return (h);
}

/*
 * balloc()
 *
 * Inputs:
 *       heap to allocate from
 *
 * Output:
 *       pointer to usable memory from the allocator
 *
 * Side Effects:
 *       attempts to allocate more memory if it runs out
 */
void *balloc(heap_t *h)
{
	node_t *walker;
	node_t *new_node;
	block_t *b;

	if (h == NULL)
	{
		blockheap_fail("balloc(): called against nonexistant heap object");
	}

	if (h->freecount == 0)
	{
		if (!newblock(h))
		{
			/* newblock failed, lets try to clean up our mess */
			bdispose(h);

			if (h->freecount == 0)
				blockheap_fail("newblock() failed and garbage collection didn't do anything");
		}
	}

	LIST_FOREACH(walker, h->blocks.head)
	{
		b = walker->data;

		if (LIST_LENGTH(&b->free_list) > 0)
		{
			h->freecount--;
			new_node = b->free_list.head;
			node_move(new_node, &b->free_list, &b->used_list);
			if (new_node->data == NULL)
				blockheap_fail("new_node->data is NULL and that shouldn't happen!!!");
			memset(new_node->data, 0, h->chunk_size);
			return (new_node->data);
		}
	}
	blockheap_fail("balloc() failed, giving up");
	return NULL;
}

/*
 * bfree()
 *
 * Inputs:
 *       parent heap, memory location to release
 *
 * Output:
 *       TRUE on success, FALSE on failure
 *
 * Side Effects:
 *       A memory chunk is marked as free.
 */
boolean_t bfree(heap_t *h, void *ptr)
{
	block_t *block;
	chunk_t *chunk;

	if (h == NULL)
	{
		slog(LG_DEBUG, "balloc.c:bfree() h == NULL");
		return FALSE;
	}

	if (ptr == NULL)
	{
		slog(LG_DEBUG, "balloc.c:bfree() ptr == NULL");
		return FALSE;
	}

	chunk = (void *)((size_t) ptr - sizeof(chunk_t));

	if (chunk->parent == NULL)
		blockheap_fail("chunk->parent == NULL, not a valid chunk?");

	block = chunk->parent;
	h->freecount++;
	node_move(&chunk->self, &block->used_list, &block->free_list);
	return TRUE;
}

/*
 * bdispose()
 *
 * Inputs:
 *       heap to garbage collect
 *
 * Output:
 *       TRUE if successful, FALSE if not
 *
 * Side Effects:
 *       performs garbage collection
 */
static boolean_t bdispose(heap_t *h)
{
	node_t *n, *n2;
	block_t *b;

	if (h == NULL)
		return FALSE;

	if (h->freecount < h->elements || h->allocated == 1)
		return TRUE;

	LIST_FOREACH_SAFE(n, n2, h->blocks.head)
	{
		b = n->data;
 
		if ((LIST_LENGTH(&b->free_list) == h->elements) != 0)
		{
			free_block(b->element, b->alloc_size);
			node_del(n, &h->blocks);
			free(b);
			h->allocated--;
			h->freecount -= h->elements;
		}
	}

	return TRUE;
}

/*
 * bdelete()
 *
 * Inputs:
 *       block heap allocator to shut down/dispose
 *
 * Output:
 *       TRUE on success, FALSE on failure
 *
 * Side Effects:
 *       a block allocator is destroyed
 */
boolean_t bdelete(heap_t *h)
{
	node_t *n, *n2;
	block_t *b;

	if (h == NULL)
		return FALSE;

	LIST_FOREACH_SAFE(n, n2, h->blocks.head)
	{
		b = n->data;
		free_block(b->element, b->alloc_size);
		node_del(n, &h->blocks);
		free(b);
	}

	node_del(&h->hlist, &heap_lists);
	free(h);

	return (0);
}

/*
 * bstat()
 *
 * Inputs:
 *       heap_t object, pointers to bused, bfree, bmemusage counters
 *
 * Outputs:
 *       none
 *
 * Side Effects:
 *       overwrites values stored in pointers, be warned.
 */
void bstat(heap_t *h, size_t *bused, size_t *bavail, size_t *bmemusage)
{
	size_t used;
	size_t freem;
	size_t memusage;

	if (h == NULL)
		return;

	freem = h->freecount;
	used = (h->allocated * h->elements) - h->freecount;
	memusage = used * (h->chunk_size + sizeof(chunk_t));

	if (bused != NULL)
		*bused = used;
	if (bavail != NULL)
		*bavail = freem;
	if (bmemusage != NULL)
		*bmemusage = memusage;
}
