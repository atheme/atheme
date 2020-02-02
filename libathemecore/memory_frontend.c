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

#include <atheme.h>
#include "internal.h"

#define ATHEME_MEMORY_FRONTEND_C        1

#ifndef SIGUSR1
#  define RAISE_EXCEPTION               abort()
#else /* !SIGUSR1 */
#  define RAISE_EXCEPTION               do { raise(SIGUSR1); abort(); } while (0)
#endif /* SIGUSR1 */

#if !defined(HAVE_TIMINGSAFE_BCMP) && !defined(HAVE_TIMINGSAFE_MEMCMP) && !defined(HAVE_CONSTTIME_MEMEQUAL)
#  ifdef HAVE_LIBSODIUM_MEMCMP
#    include <sodium/utils.h>
#  else /* HAVE_LIBSODIUM_MEMCMP */
#    ifdef HAVE_LIBCRYPTO_MEMCMP
#      include <openssl/crypto.h>
#    else /* HAVE_LIBCRYPTO_MEMCMP */
#      ifdef HAVE_LIBNETTLE_MEMEQL
#        include <nettle/memops.h>
#      endif /* !HAVE_LIBNETTLE_MEMEQL */
#    endif /* !HAVE_LIBCRYPTO_MEMCMP */
#  endif /* !HAVE_LIBSODIUM_MEMCMP */
#endif /* !HAVE_TIMINGSAFE_BCMP && !HAVE_TIMINGSAFE_MEMCMP */

#if !defined(HAVE_MEMSET_S) && !defined(HAVE_EXPLICIT_MEMSET) && !defined(HAVE_EXPLICIT_BZERO)
#  ifdef HAVE_LIBSODIUM_MEMZERO
#    include <sodium/utils.h>
#  endif /* HAVE_LIBSODIUM_MEMZERO */
#endif /* !HAVE_MEMSET_S && !HAVE_EXPLICIT_MEMSET && !HAVE_EXPLICIT_BZERO */

#ifdef ATHEME_ENABLE_SODIUM_MALLOC
#  include "memory_fe_sodium.c"
#else /* ATHEME_ENABLE_SODIUM_MALLOC */
#  include "memory_fe_system.c"
#endif /* !ATHEME_ENABLE_SODIUM_MALLOC */

void * ATHEME_FATTR_ALLOC_SIZE(1) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL
smalloc(const size_t len)
{
	return scalloc(1, len);
}

void * ATHEME_FATTR_ALLOC_SIZE_PRODUCT(2, 3) ATHEME_FATTR_WUR
sreallocarray(void *const restrict ptr, const size_t num, const size_t len)
{
	const size_t product = (num * len);

	// Check for overflow
	if (product < num || product < len || num > (SIZE_MAX / len))
		RAISE_EXCEPTION;

	return srealloc(ptr, product);
}

int ATHEME_FATTR_WUR
smemcmp(const void *const ptr1, const void *const ptr2, const size_t len)
{
#ifdef HAVE_TIMINGSAFE_BCMP

	return timingsafe_bcmp(ptr1, ptr2, len);

#else /* HAVE_TIMINGSAFE_BCMP */
#  ifdef HAVE_TIMINGSAFE_MEMCMP

	return timingsafe_memcmp(ptr1, ptr2, len);

#  else /* HAVE_TIMINGSAFE_MEMCMP */
#    ifdef HAVE_CONSTTIME_MEMEQUAL

	return !consttime_memequal(ptr1, ptr2, len);

#    else /* HAVE_CONSTTIME_MEMEQUAL */
#      ifdef HAVE_LIBSODIUM_MEMCMP

	return sodium_memcmp(ptr1, ptr2, len);

#      else /* HAVE_LIBSODIUM_MEMCMP */
#        ifdef HAVE_LIBCRYPTO_MEMCMP

	return CRYPTO_memcmp(ptr1, ptr2, len);

#        else /* HAVE_LIBCRYPTO_MEMCMP */
#          ifdef HAVE_LIBNETTLE_MEMEQL

	return !nettle_memeql_sec(ptr1, ptr2, len);

#          else /* HAVE_LIBNETTLE_MEMEQL */

#            warning "No third-party cryptographically-secure constant-time memory comparison function is available"

	/* WARNING:
	 *   This is highly liable to be optimised out with
	 *   LTO builds, but it's still better than nothing.
	 */
	volatile const unsigned char *val1 = (volatile const unsigned char *) ptr1;
	volatile const unsigned char *val2 = (volatile const unsigned char *) ptr2;
	volatile int result = 0;

	for (size_t i = 0; i < len; i++)
		result |= (int) ((*val1++) ^ (*val2++));

	return result;

#          endif /* !HAVE_LIBNETTLE_MEMEQL */
#        endif /* !HAVE_LIBCRYPTO_MEMCMP */
#      endif /* !HAVE_LIBSODIUM_MEMCMP */
#    endif /* !HAVE_CONSTTIME_MEMEQUAL */
#  endif /* !HAVE_TIMINGSAFE_MEMCMP */
#endif /* !HAVE_TIMINGSAFE_BCMP */
}

void
smemzero(void *const restrict ptr, const size_t len)
{
	if (! (ptr && len))
		return;

#ifdef HAVE_MEMSET_S

	if (memset_s(ptr, len, 0x00, len) != 0)
		RAISE_EXCEPTION;

#else /* HAVE_MEMSET_S */
#  ifdef HAVE_EXPLICIT_BZERO

	(void) explicit_bzero(ptr, len);

#  else /* HAVE_EXPLICIT_BZERO */
#    ifdef HAVE_EXPLICIT_MEMSET

	(void) explicit_memset(ptr, 0x00, len);

#    else /* HAVE_EXPLICIT_MEMSET */
#      ifdef HAVE_LIBSODIUM_MEMZERO

	(void) sodium_memzero(ptr, len);

#      else /* HAVE_LIBSODIUM_MEMZERO */

	/* Indirect memset(3) through a volatile function pointer should hopefully prevent dead-store elimination
	 * removing the call. This may not work if Atheme IRC Services is built with Link Time Optimisation, because
	 * the compiler may be able to prove (for a given definition of proof) that the pointer always points to
	 * memset(3); LTO lets the compiler analyse every compilation unit, not just this one. Alas, the C standard
	 * only requires the compiler to read the value of the pointer, not make the function call through it; so if
	 * it reads it and determines that it still points to memset(3), it can still decide not to call it. To
	 * hopefully prevent the compiler making assumptions about what it points to, it is not located in this
	 * compilation unit; and this unit (and the unit it is located in) is part of a library, so in theory a
	 * consumer of this library could modify this extern variable to point to anything. Still, the C standard
	 * does not guarantee that this will work, and a sufficiently clever compiler may still remove the smemzero
	 * function calls from modules that use this library if Full LTO is used, because nothing in this program
	 * or any of its modules sets the function pointer to any other value.
	 *
	 * Clang <= 7.0 with/without Thin LTO does not remove any calls; other compilers & situations are untested.
	 */

	(void) volatile_memset(ptr, 0x00, len);

#      endif /* !HAVE_LIBSODIUM_MEMZERO */
#    endif /* !HAVE_EXPLICIT_MEMSET */
#  endif /* !HAVE_EXPLICIT_BZERO */
#endif /* !HAVE_MEMSET_S */
}

void * ATHEME_FATTR_MALLOC
smemdup(const void *const restrict ptr, const size_t len)
{
	if (! ptr || ! len)
		return NULL;

	void *const buf = smalloc(len);

	return memcpy(buf, ptr, len);
}

char * ATHEME_FATTR_MALLOC
sstrdup(const char *const restrict ptr)
{
	if (! ptr)
		return NULL;

	const size_t len = strlen(ptr);
	char *const buf = smalloc(len + 1);

	if (len)
		(void) memcpy(buf, ptr, len);

	return buf;
}

char * ATHEME_FATTR_MALLOC
sstrndup(const char *const restrict ptr, const size_t maxlen)
{
	if (! ptr)
		return NULL;

	const size_t len = strnlen(ptr, maxlen);
	char *const buf = smalloc(len + 1);

	if (len)
		(void) memcpy(buf, ptr, len);

	return buf;
}
