/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
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

void
sfree(void *const restrict ptr)
{
	(void) free(ptr);
}

void
smemzerofree(void *const restrict ptr, const size_t len)
{
	(void) smemzero(ptr, len);
	(void) sfree(ptr);
}

void * ATHEME_FATTR_ALLOC_SIZE_PRODUCT(1, 2) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL
scalloc(const size_t num, const size_t len)
{
	void *const buf = calloc(num, len);

	if (! buf)
		RAISE_EXCEPTION;

	return buf;
}

void * ATHEME_FATTR_ALLOC_SIZE(2) ATHEME_FATTR_WUR
srealloc(void *const restrict ptr, const size_t len)
{
	void *const buf = realloc(ptr, len);

	if (len && ! buf)
		RAISE_EXCEPTION;

	return buf;
}
