/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the block allocator.
 *
 * $Id: balloc.c 3325 2005-10-31 03:27:49Z nenolod $
 */

#include <org.atheme.claro.base>

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

#ifndef HAVE_MMAP
#define MAP_PRIVATE 0
#define MAP_ANON 0
#define MAP_FAILED 0
#define PROT_READ 0
#define PROT_WRITE 0
#endif

static int newblock(BlockHeap *bh);
static int BlockHeapGarbageCollect(BlockHeap *);
static void block_heap_gc(void *unused);
static list_t heap_lists;

#define blockheap_fail(x) _blockheap_fail(x, __FILE__, __LINE__)

static void _blockheap_fail(const char *reason, const char *file, int line)
{
	clog(LG_INFO, "Blockheap failure: %s (%s:%d)", reason, file, line);
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
	
	memset(ptr, 0, len);
	
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

void initBlockHeap(void)
{
	event_add("block_heap_gc", block_heap_gc, NULL, 60);
}

/*
 * static void *get_block(size_t size)
 * 
 * Input: Size of block to allocate
 * Output: Pointer to new block
 * Side Effects: None
 */
static void *get_block(size_t size)
{
	void *ptr;
	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	
	if (ptr == MAP_FAILED)
		ptr = NULL;
	
	return (ptr);
}


static void block_heap_gc(void *unused)
{
	node_t *ptr, *tptr;

	LIST_FOREACH_SAFE(ptr, tptr, heap_lists.head)
		BlockHeapGarbageCollect(ptr->data);
}

/* ************************************************************************ */
/* FUNCTION DOCUMENTATION:                                                  */
/*    newblock                                                              */
/* Description:                                                             */
/*    Allocates a new block for addition to a blockheap                     */
/* Parameters:                                                              */
/*    bh (IN): Pointer to parent blockheap.                                 */
/* Returns:                                                                 */
/*    0 if successful, 1 if not                                             */
/* ************************************************************************ */

static int newblock(BlockHeap *bh)
{
	MemBlock *newblk;
	Block *b;
	unsigned long i;
	void *offset;

	/* Setup the initial data structure. */
	b = (Block *)scalloc(1, sizeof(Block));

	if (b == NULL)
		return (1);

	b->free_list.head = b->free_list.tail = NULL;
	b->used_list.head = b->used_list.tail = NULL;
	b->next = bh->base;

	b->alloc_size = (bh->elemsPerBlock + 1) * (bh->elemSize + sizeof(MemBlock));

	b->elems = get_block(b->alloc_size);

	if (b->elems == NULL)
		return (1);

	offset = b->elems;

	/* Setup our blocks now */
	for (i = 0; i < bh->elemsPerBlock; i++)
	{
		void *data;
		newblk = (void *)offset;
		newblk->block = b;
#ifdef DEBUG_BALLOC
		newblk->magic = BALLOC_MAGIC;
#endif
		data = (void *)((size_t) offset + sizeof(MemBlock));
		newblk->block = b;
		node_add(data, &newblk->self, &b->free_list);
		offset = (unsigned char *)((unsigned char *)offset + bh->elemSize + sizeof(MemBlock));
	}

	++bh->blocksAllocated;
	bh->freeElems += bh->elemsPerBlock;
	bh->base = b;

	return (0);
}


/* ************************************************************************ */
/* FUNCTION DOCUMENTATION:                                                  */
/*    BlockHeapCreate                                                       */
/* Description:                                                             */
/*   Creates a new blockheap from which smaller blocks can be allocated.    */
/*   Intended to be used instead of multiple calls to malloc() when         */
/*   performance is an issue.                                               */
/* Parameters:                                                              */
/*   elemsize (IN):  Size of the basic element to be stored                 */
/*   elemsperblock (IN):  Number of elements to be stored in a single block */
/*         of memory.  When the blockheap runs out of free memory, it will  */
/*         allocate elemsize * elemsperblock more.                          */
/* Returns:                                                                 */
/*   Pointer to new BlockHeap, or NULL if unsuccessful                      */
/* ************************************************************************ */
BlockHeap *BlockHeapCreate(size_t elemsize, int elemsperblock)
{
	BlockHeap *bh;

	/* Catch idiotic requests up front */
	if ((elemsize <= 0) || (elemsperblock <= 0))
	{
		blockheap_fail("Attempting to BlockHeapCreate idiotic sizes");
	}

	/* Allocate our new BlockHeap */
	bh = (BlockHeap *)scalloc(1, sizeof(BlockHeap));
	
	if (bh == NULL)
	{
		clog(LG_INFO, "Attempt to calloc() failed: (%s:%d)", __FILE__, __LINE__);
		runflags |= RF_SHUTDOWN;
	}

	if ((elemsize % sizeof(void *)) != 0)
	{
		/* Pad to even pointer boundary */
		elemsize += sizeof(void *);
		elemsize &= ~(sizeof(void *) - 1);
	}

	bh->elemSize = elemsize;
	bh->elemsPerBlock = elemsperblock;
	bh->blocksAllocated = 0;
	bh->freeElems = 0;
	bh->base = NULL;

	/* Be sure our malloc was successful */
	if (newblock(bh))
	{
		if (bh != NULL)
			free(bh);
		clog(LG_INFO, "newblock() failed");
		runflags |= RF_SHUTDOWN;
	}

	if (bh == NULL)
	{
		blockheap_fail("bh == NULL when it shouldn't be");
	}
	node_add(bh, &bh->hlist, &heap_lists);
	return (bh);
}

/* ************************************************************************ */
/* FUNCTION DOCUMENTATION:                                                  */
/*    BlockHeapAlloc                                                        */
/* Description:                                                             */
/*    Returns a pointer to a struct within our BlockHeap that's free for    */
/*    the taking.                                                           */
/* Parameters:                                                              */
/*    bh (IN):  Pointer to the Blockheap.                                   */
/* Returns:                                                                 */
/*    Pointer to a structure (void *), or NULL if unsuccessful.             */
/* ************************************************************************ */

void *BlockHeapAlloc(BlockHeap *bh)
{
	Block *walker;
	node_t *new_node;

	if (bh == NULL)
	{
		blockheap_fail("Cannot allocate if bh == NULL");
	}

	if (bh->freeElems == 0)
	{
		/* Allocate new block and assign */
		/* newblock returns 1 if unsuccessful, 0 if not */

		if (newblock(bh))
		{
			/* That didn't work..try to garbage collect */
			BlockHeapGarbageCollect(bh);
			if (bh->freeElems == 0)
			{
				clog(LG_INFO, "newblock() failed and garbage collection didn't help");
				runflags |= RF_SHUTDOWN;
			}
		}
	}

	for (walker = bh->base; walker != NULL; walker = walker->next)
	{
		if (LIST_LENGTH(&walker->free_list) > 0)
		{
			bh->freeElems--;
			new_node = walker->free_list.head;
			node_move(new_node, &walker->free_list, &walker->used_list);
			if (new_node->data == NULL)
				blockheap_fail("new_node->data is NULL and that shouldn't happen!!!");
			memset(new_node->data, 0, bh->elemSize);
			return (new_node->data);
		}
	}
	blockheap_fail("BlockHeapAlloc failed, giving up");
	return NULL;
}


/* ************************************************************************ */
/* FUNCTION DOCUMENTATION:                                                  */
/*    BlockHeapFree                                                         */
/* Description:                                                             */
/*    Returns an element to the free pool, does not free()                  */
/* Parameters:                                                              */
/*    bh (IN): Pointer to BlockHeap containing element                      */
/*    ptr (in):  Pointer to element to be "freed"                           */
/* Returns:                                                                 */
/*    0 if successful, 1 if element not contained within BlockHeap.         */
/* ************************************************************************ */
int BlockHeapFree(BlockHeap *bh, void *ptr)
{
	Block *block;
	struct MemBlock *memblock;

	if (bh == NULL)
	{

		clog(LG_DEBUG, "balloc.c:BlockHeapFree() bh == NULL");
		return (1);
	}

	if (ptr == NULL)
	{
		clog(LG_DEBUG, "balloc.c:BlockHeapFree() ptr == NULL");
		return (1);
	}

	memblock = (void *)((size_t) ptr - sizeof(MemBlock));
#ifdef DEBUG_BALLOC
	if (memblock->magic != BALLOC_MAGIC)
	{
		blockheap_fail("memblock->magic != BALLOC_MAGIC");
		runflags |= RF_SHUTDOWN;
	}
#endif
	if (memblock->block == NULL)
	{
		blockheap_fail("memblock->block == NULL, not a valid block?");
		runflags |= RF_SHUTDOWN;
	}

	block = memblock->block;
	bh->freeElems++;
	node_move(&memblock->self, &block->used_list, &block->free_list);
	return (0);
}

/* ************************************************************************ */
/* FUNCTION DOCUMENTATION:                                                  */
/*    BlockHeapGarbageCollect                                               */
/* Description:                                                             */
/*    Performs garbage collection on the block heap.  Any blocks that are   */
/*    completely unallocated are removed from the heap.  Garbage collection */
/*    will never remove the root node of the heap.                          */
/* Parameters:                                                              */
/*    bh (IN):  Pointer to the BlockHeap to be cleaned up                   */
/* Returns:                                                                 */
/*   0 if successful, 1 if bh == NULL                                       */
/* ************************************************************************ */
static int BlockHeapGarbageCollect(BlockHeap *bh)
{
	Block *walker, *last;
	if (bh == NULL)
	{
		return (1);
	}

	if (bh->freeElems < bh->elemsPerBlock || bh->blocksAllocated == 1)
	{
		/* There couldn't possibly be an entire free block.  Return. */
		return (0);
	}

	last = NULL;
	walker = bh->base;

	while (walker != NULL)
	{
		if ((LIST_LENGTH(&walker->free_list) == bh->elemsPerBlock) != 0)
		{
			free_block(walker->elems, walker->alloc_size);
			if (last != NULL)
			{
				last->next = walker->next;
				if (walker != NULL)
					free(walker);
				walker = last->next;
			}
			else
			{
				bh->base = walker->next;
				if (walker != NULL)
					free(walker);
				walker = bh->base;
			}
			bh->blocksAllocated--;
			bh->freeElems -= bh->elemsPerBlock;
		}
		else
		{
			last = walker;
			walker = walker->next;
		}
	}
	return (0);
}

/* ************************************************************************ */
/* FUNCTION DOCUMENTATION:                                                  */
/*    BlockHeapDestroy                                                      */
/* Description:                                                             */
/*    Completely free()s a BlockHeap.  Use for cleanup.                     */
/* Parameters:                                                              */
/*    bh (IN):  Pointer to the BlockHeap to be destroyed.                   */
/* Returns:                                                                 */
/*   0 if successful, 1 if bh == NULL                                       */
/* ************************************************************************ */
int BlockHeapDestroy(BlockHeap *bh)
{
	Block *walker, *next;

	if (bh == NULL)
	{
		return (1);
	}

	for (walker = bh->base; walker != NULL; walker = next)
	{
		next = walker->next;
		free_block(walker->elems, walker->alloc_size);
		if (walker != NULL)
			free(walker);
	}
	node_del(&bh->hlist, &heap_lists);
	free(bh);
	return (0);
}

void BlockHeapUsage(BlockHeap *bh, size_t * bused, size_t * bfree, size_t * bmemusage)
{
	size_t used;
	size_t freem;
	size_t memusage;
	if (bh == NULL)
	{
		return;
	}

	freem = bh->freeElems;
	used = (bh->blocksAllocated * bh->elemsPerBlock) - bh->freeElems;
	memusage = used * (bh->elemSize + sizeof(MemBlock));

	if (bused != NULL)
		*bused = used;
	if (bfree != NULL)
		*bfree = freem;
	if (bmemusage != NULL)
		*bmemusage = memusage;
}
