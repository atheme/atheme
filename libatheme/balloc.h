/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for the block allocator.
 *
 * $Id: balloc.h 3001 2005-10-19 04:40:25Z nenolod $
 */

#ifndef BALLOC_H
#define BALLOC_H

struct Block
{
  size_t alloc_size;
  struct Block *next;     /* Next in our chain of blocks */
  void *elems;            /* Points to allocated memory */
  list_t free_list;
  list_t used_list;
};
typedef struct Block Block;

struct MemBlock
{
#ifdef DEBUG_BALLOC
  unsigned long magic;
#endif
  node_t self;
  Block *block;           /* Which block we belong to */
};

typedef struct MemBlock MemBlock;

/* information for the root node of the heap */
struct BlockHeap
{
  node_t hlist;
  size_t elemSize;        /* Size of each element to be stored */
  unsigned long elemsPerBlock;    /* Number of elements per block */
  unsigned long blocksAllocated;  /* Number of blocks allocated */
  unsigned long freeElems;                /* Number of free elements */
  Block *base;            /* Pointer to first block */
};
typedef struct BlockHeap BlockHeap;

#endif
