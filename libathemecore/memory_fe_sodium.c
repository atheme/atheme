/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * memory.c: Memory wrappers
 */

#ifndef ATHEME_MEMORY_FRONTEND_C
#  error "Do not compile me directly; compile memory_frontend.c instead"
#endif /* !ATHEME_MEMORY_FRONTEND_C */

#include <sodium/utils.h>

struct sodium_memblock
{
	mowgli_node_t   node;
	void *          ptr;
	size_t          len;
};

static mowgli_list_t *sodium_memblocks = NULL;

static inline void
make_sodium_memblocks_ro(void)
{
	mowgli_node_t *n;

	if (sodium_mprotect_readonly(sodium_memblocks) != 0)
		abort();

	MOWGLI_ITER_FOREACH(n, sodium_memblocks->head)
	{
		struct sodium_memblock *const mptr = n->data;

		if (sodium_mprotect_readonly(mptr) != 0)
			abort();
	}
}

static inline void
make_sodium_memblocks_rw(void)
{
	mowgli_node_t *n;

	if (sodium_mprotect_readwrite(sodium_memblocks) != 0)
		abort();

	MOWGLI_ITER_FOREACH(n, sodium_memblocks->head)
	{
		struct sodium_memblock *const mptr = n->data;

		if (sodium_mprotect_readwrite(mptr) != 0)
			abort();
	}
}

static void
make_sodium_memblock(void *const restrict ptr, const size_t len)
{
	if (! ptr || ! len)
		/* bug in this code */
		abort();

	if (! sodium_memblocks)
	{
		if (! (sodium_memblocks = sodium_malloc(sizeof *sodium_memblocks)))
			/* no free memory? */
			abort();

		(void) memset(sodium_memblocks, 0x00, sizeof *sodium_memblocks);
	}

	struct sodium_memblock *const mptr = sodium_malloc(sizeof *mptr);

	if (! mptr)
		/* no free memory? */
		abort();

	mptr->ptr = ptr;
	mptr->len = len;

	(void) make_sodium_memblocks_rw();
	(void) mowgli_node_add(mptr, &mptr->node, sodium_memblocks);
	(void) make_sodium_memblocks_ro();
}

static struct sodium_memblock *
find_sodium_memblock(void *const restrict ptr)
{
	if (! ptr)
		/* bug in this code */
		abort();

	if (! sodium_memblocks)
		/* srealloc(ptr!=NULL) or sfree(ptr!=NULL) before any s(c|m|re)alloc() calls */
		abort();

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sodium_memblocks->head)
	{
		struct sodium_memblock *const mptr = n->data;

		if (mptr->ptr == ptr)
			return mptr;
	}

	return NULL;
}

static void
free_sodium_memblock(struct sodium_memblock *const restrict mptr)
{
	if (! mptr)
		/* bug in this code */
		abort();

	if (! sodium_memblocks)
		/* srealloc(ptr!=NULL,len==0) or sfree(ptr!=NULL) before any s(c|m|re)alloc() calls */
		abort();

	(void) make_sodium_memblocks_rw();
	(void) mowgli_node_delete(&mptr->node, sodium_memblocks);
	(void) make_sodium_memblocks_ro();

	(void) sodium_free(mptr);
}

void
sfree(void *const restrict ptr)
{
	if (! ptr)
		/* free(NULL) is well-defined to do nothing */
		return;

	struct sodium_memblock *const mptr = find_sodium_memblock(ptr);

	if (! mptr)
		/* sfree() on something that s(c|m|re)alloc() didn't provide or double-free */
		abort();

	(void) free_sodium_memblock(mptr);
	(void) sodium_free(ptr);
}

void * ATHEME_FATTR_ALLOC_SIZE_PRODUCT(1, 2) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL
scalloc(const size_t num, const size_t len)
{
	if (! num || ! len)
		/* calloc(x, y) for (x * y) == 0 should return NULL but that would break us */
		abort();

	if (len >= (size_t) (((size_t) SIZE_MAX) / num))
		/* (num * len) would overflow */
		abort();

	const size_t totalsz = (num * len);

	void *const buf = sodium_malloc(totalsz);

	if (! buf)
		/* no free memory? */
		abort();

	(void) make_sodium_memblock(buf, totalsz);

	return memset(buf, 0x00, totalsz);
}

void * ATHEME_FATTR_ALLOC_SIZE(2) ATHEME_FATTR_WUR
srealloc(void *const restrict ptr, const size_t len)
{
	if (! ptr)
		/* realloc(NULL, x) == malloc(x) */
		return smalloc(len);

	struct sodium_memblock *const mptr = find_sodium_memblock(ptr);

	if (! mptr)
		abort();

	if (! len)
	{
		/* realloc(ptr, 0) == free(ptr) */

		(void) free_sodium_memblock(mptr);
		(void) sodium_free(ptr);

		return NULL;
	}

	if (len == mptr->len)
		/* realloc() to same size as existing alloc -- do nothing */
		return ptr;

	void *const buf = smalloc(len);

	if (len > mptr->len)
		(void) memcpy(buf, ptr, mptr->len);
	else
		(void) memcpy(buf, ptr, len);

	(void) free_sodium_memblock(mptr);
	(void) sodium_free(ptr);

	return buf;
}
