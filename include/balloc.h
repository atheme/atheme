/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * The block allocator.
 *
 */

#ifndef BALLOC_H
#define BALLOC_H

struct BlockHeap;
typedef struct BlockHeap BlockHeap;

E int BlockHeapFree(BlockHeap *bh, void *ptr);
E void *BlockHeapAlloc(BlockHeap *bh);

E BlockHeap *BlockHeapCreate(size_t elemsize, int elemsperblock);
E int BlockHeapDestroy(BlockHeap *bh);

E void initBlockHeap(void);
E void BlockHeapUsage(BlockHeap *bh, size_t * bused, size_t * bfree,
                      size_t * bmemusage);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
