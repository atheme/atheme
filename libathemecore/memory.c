/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * memory.c: Memory wrappers
 */

#include "atheme.h"

#ifndef SIGUSR1
#  define RAISE_EXCEPTION       abort()
#else
#  define RAISE_EXCEPTION       raise(SIGUSR1)
#endif

/* does malloc()'s job and dies if it fails
 *
 * Note that this function *MUST* RETURN ZERO-INITIALIZED MEMORY
 * Parts of the codebase assume this is so and will malfunction otherwise
 */
void * ATHEME_FATTR_MALLOC
smalloc(const size_t len)
{
	return scalloc(1, len);
}

/* does calloc()'s job and dies if it fails
 *
 * Note that this function *MUST* RETURN ZERO-INITIALIZED MEMORY
 * Parts of the codebase assume this is so and will malfunction otherwise
 */
void * ATHEME_FATTR_MALLOC
scalloc(const size_t num, const size_t len)
{
	void *const buf = calloc(num, len);

	if (! buf)
		RAISE_EXCEPTION;

	return buf;
}

/* does realloc()'s job and dies if it fails */
void * /* ATHEME_FATTR_MALLOC is not applicable -- may return same ptr */
srealloc(void *const restrict ptr, const size_t len)
{
	void *const buf = realloc(ptr, len);

	if (len && ! buf)
		RAISE_EXCEPTION;

	return buf;
}

/* does strdup()'s job, only with the above memory functions */
char * ATHEME_FATTR_MALLOC
sstrdup(const char *const restrict ptr)
{
	if (! ptr)
		return NULL;

	const size_t len = strlen(ptr);
	char *const buf = smalloc(len + 1);

	(void) memcpy(buf, ptr, len);
	return buf;
}

/* does strndup()'s job, only with the above memory functions */
char * ATHEME_FATTR_MALLOC
sstrndup(const char *const restrict ptr, const size_t len)
{
	if (! ptr)
		return NULL;

	char *const buf = smalloc(len + 1);

	(void) mowgli_strlcpy(buf, ptr, len + 1);
	return buf;
}
