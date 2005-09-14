/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for the block allocator.
 *
 * $Id: balloc.h 2235 2005-09-14 07:29:13Z nenolod $
 */

#ifndef BALLOC_H
#define BALLOC_H

struct block_
{
	size_t alloc_size;
	node_t self;
	void  *element;            /* Points to allocated memory */
	list_t free_list;
	list_t used_list;
};
typedef struct block_ block_t;

struct chunk_
{
	node_t   self;
	block_t *parent;           /* Which block we belong to */
};
typedef struct chunk_ chunk_t;

/* information for the root node of the heap */
struct heap_
{
	node_t hlist;
	char *name;
	size_t chunk_size;        /* Size of each element to be stored */
	unsigned long elements;   /* Number of elements per block */
	unsigned long allocated;  /* Number of blocks allocated */
	unsigned long freecount;  /* Number of free elements */
	list_t blocks;            /* old blocks.head == base */
};
typedef struct heap_ heap_t;

extern void init_balloc(void);
extern heap_t *bcreate(char *name, size_t elemsize, int elemsperblock);
extern void *balloc(heap_t *h);
extern boolean_t bfree(heap_t *h, void *ptr);
extern boolean_t bdelete(heap_t *h);
extern void bstat(heap_t *h, size_t *bused, size_t *bavail, size_t *bmemusage);

#endif
