/*
 * atheme-services: A collection of minimalist IRC services
 * memory.c: Memory wrappers
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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
 */

#include "atheme.h"

#ifndef SIGUSR1
#  define RAISE_EXCEPTION       abort()
#else
#  define RAISE_EXCEPTION       raise(SIGUSR1)
#endif

/* does free()'s job
 *
 * This is only here to balance out the custom malloc()/calloc()/realloc()
 * below. This will be useful if the allocators below are ever repurposed
 * to do something different (e.g. use a hardened memory alloator library).
 */
void
sfree(void *const restrict ptr)
{
	(void) free(ptr);
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
