/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * memory_fe_sodium.c: An extremely-paranoid hardened memory allocator
 *                     built on top of the libsodium cryptographic library
 */

#ifndef ATHEME_MEMORY_FRONTEND_C
#  error "Do not compile me directly; compile memory_frontend.c instead"
#endif /* !ATHEME_MEMORY_FRONTEND_C */

#include <sodium/utils.h>

/* Stores memory allocation information (pointer to allocated memory, amount of memory allocated). This is
 * entered into a list of memory allocations. This is all necessary to correctly implement srealloc() below.
 */
struct sodium_memblock
{
	mowgli_node_t   node;
	void *          ptr;
	size_t          len;
};

// The list of memory allocations
static mowgli_list_t *sodium_memblocks = NULL;

static void
make_sodium_memblock(void *const restrict ptr, const size_t len)
{
	if (! ptr || ! len)
		// Bug in this code
		abort();

	if (! sodium_memblocks)
	{
		// Allocate it
		if (! (sodium_memblocks = sodium_malloc(sizeof *sodium_memblocks)))
			// No free memory? Library failure?
			abort();

		// Erase it
		(void) memset(sodium_memblocks, 0x00, sizeof *sodium_memblocks);

		// Test whether making it read-only works
		if (sodium_mprotect_readonly(sodium_memblocks) != 0)
			abort();
	}

	// Create a memory allocation infoblock
	struct sodium_memblock *const mptr = sodium_malloc(sizeof *mptr);

	if (! mptr)
		// No free memory? Library failure?
		abort();

	// Store the memory allocation info
	mptr->ptr = ptr;
	mptr->len = len;

	// Grab the current head infoblock from the list (if any)
	const mowgli_node_t *const tn = sodium_memblocks->head;

	// Make the current head infoblock writeable (so its `node.prev` pointer can be modified)
	if (tn && sodium_mprotect_readwrite(tn->data) != 0)
		abort();

	// Make the allocations list writeable
	if (sodium_mprotect_readwrite(sodium_memblocks) != 0)
		abort();

	// Add the new infoblock to the head of the allocations list
	(void) mowgli_node_add_head(mptr, &mptr->node, sodium_memblocks);

	// Make the allocations list read-only
	if (sodium_mprotect_readonly(sodium_memblocks) != 0)
		abort();

	// Make the previous head infoblock read-only
	if (tn && sodium_mprotect_readonly(tn->data) != 0)
		abort();

	// Finally, make the new infoblock read-only
	if (sodium_mprotect_readonly(mptr) != 0)
		abort();
}

static struct sodium_memblock * ATHEME_FATTR_RETURNS_NONNULL
find_sodium_memblock(void *const restrict ptr)
{
	if (! ptr)
		// Bug in this code
		abort();

	if (! sodium_memblocks)
		// An srealloc(ptr!=NULL) or sfree(ptr!=NULL) call, before any s(c|m|re)alloc() calls
		abort();

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sodium_memblocks->head)
	{
		struct sodium_memblock *const mptr = n->data;

		if (mptr->ptr == ptr)
			return mptr;
	}

	// An srealloc(ptr!=NULL) or free(ptr!=NULL) call for a ptr not allocated by us, or a double-free
	abort();
}

static void
free_sodium_memblock(struct sodium_memblock *const restrict mptr)
{
	if (! mptr)
		// Bug in this code
		abort();

	if (! sodium_memblocks)
		// An srealloc(ptr!=NULL,len==0) or sfree(ptr!=NULL) call, before any s(c|m|re)alloc() calls
		abort();

	// Grab the current infoblock's previous and next infoblocks (if any)
	const mowgli_node_t *const tp = mptr->node.prev;
	const mowgli_node_t *const tn = mptr->node.next;

	// Make the current infoblock writeable
	if (sodium_mprotect_readwrite(mptr) != 0)
		abort();

	// Make the current infoblock's previous infoblock writeable (so its `node.next` pointer can be modified)
	if (tp && sodium_mprotect_readwrite(tp->data) != 0)
		abort();

	// Make the current infoblock's next infoblock writeable (so its `node.prev` pointer can be modified)
	if (tn && sodium_mprotect_readwrite(tn->data) != 0)
		abort();

	// Make the allocations list writeable
	if (sodium_mprotect_readwrite(sodium_memblocks) != 0)
		abort();

	// Remove the current infoblock from the list
	(void) mowgli_node_delete(&mptr->node, sodium_memblocks);

	// Make the allocations list read-only
	if (sodium_mprotect_readonly(sodium_memblocks) != 0)
		abort();

	// Make the current infoblock's previous infoblock read-only
	if (tp && sodium_mprotect_readonly(tp->data) != 0)
		abort();

	// Make the current infoblock's next infoblock read-only
	if (tn && sodium_mprotect_readonly(tn->data) != 0)
		abort();

	// Finally, free the memory allocation within the current infoblock, and then the current infoblock itself
	(void) sodium_free(mptr->ptr);
	(void) sodium_free(mptr);
}

void
sfree(void *const restrict ptr)
{
	// A free(NULL) call is well-defined to do nothing
	if (! ptr)
		return;

	struct sodium_memblock *const mptr = find_sodium_memblock(ptr);

	(void) free_sodium_memblock(mptr);
}

void
smemzerofree(void *const restrict ptr, const size_t ATHEME_VATTR_UNUSED len)
{
	// No erasing here: the library does its own sodium_memzero() on the pointer given to sodium_free()
	(void) sfree(ptr);
}

void * ATHEME_FATTR_ALLOC_SIZE_PRODUCT(1, 2) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL
scalloc(const size_t num, const size_t len)
{
	if (! num || ! len)
		// A calloc(num, len) call where the product is 0 should return NULL, but codebase assumes it doesn't
		abort();

	// Allocate the memory
	void *const buf = sodium_allocarray(num, len);

	if (! buf)
		// No free memory? Library failure? Multiplication overflow?
		abort();

	// We now know that this doesn't overflow
	const size_t totalsz = (num * len);

	// Store it in the allocations list
	(void) make_sodium_memblock(buf, totalsz);

	// Erase it before returning it -- codebase assumes smalloc() and scalloc() both return zeroed memory
	return memset(buf, 0x00, totalsz);
}

void * ATHEME_FATTR_ALLOC_SIZE(2) ATHEME_FATTR_WUR
srealloc(void *const restrict ptr, const size_t len)
{
	// A realloc(NULL, len) call is equivalent to malloc(len)
	if (! ptr)
		return smalloc(len);

	struct sodium_memblock *const mptr = find_sodium_memblock(ptr);

	// A realloc(ptr, 0) call is equivalent to free(ptr)
	if (! len)
	{
		(void) free_sodium_memblock(mptr);
		return NULL;
	}

	if (len == mptr->len)
		// Asked to reallocate to the same size -- do nothing
		return ptr;

	// Perform a new allocation
	void *const buf = smalloc(len);

	// Copy the data in the existing allocation to it
	if (len > mptr->len)
		(void) memcpy(buf, ptr, mptr->len);
	else
		(void) memcpy(buf, ptr, len);

	// Free the existing allocation
	(void) free_sodium_memblock(mptr);

	// Return the new allocation
	return buf;
}
