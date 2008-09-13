/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_heap.h: Heap allocation.
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

#ifndef __MOWGLI_HEAP_H__
#define __MOWGLI_HEAP_H__

typedef struct mowgli_heap_ mowgli_heap_t;
typedef struct mowgli_block_ mowgli_block_t;

/* Flag for mowgli_heap_create */
#define BH_DONTCARE 0

#define BH_NOW 1
#define BH_LAZY 0

/* Functions for heaps */
extern mowgli_heap_t *mowgli_heap_create(size_t elem_size, size_t mowgli_heap_elems, unsigned int flags);
extern mowgli_heap_t *mowgli_heap_create_full(size_t elem_size, size_t mowgli_heap_elems, unsigned int flags,
	mowgli_allocation_policy_t *allocator);
extern void mowgli_heap_destroy(mowgli_heap_t *heap);

/* Functions for blocks */
extern void *mowgli_heap_alloc(mowgli_heap_t *heap);
extern void mowgli_heap_free(mowgli_heap_t *heap, void *data);

#endif

