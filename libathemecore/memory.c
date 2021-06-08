/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2019 Aaron M. D. Jones <me@aaronmdjones.net>
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

#ifndef SIGUSR1
#  define RAISE_EXCEPTION           abort()
#else /* !SIGUSR1 */
#  define RAISE_EXCEPTION           do { raise(SIGUSR1); abort(); } while (0)
#endif /* SIGUSR1 */

#if !defined(HAVE_TIMINGSAFE_BCMP) && !defined(HAVE_TIMINGSAFE_MEMCMP) && !defined(HAVE_CONSTTIME_MEMEQUAL)
#  if defined(HAVE_LIBSODIUM_MEMCMP)
#    include <sodium/utils.h>
#  elif defined(HAVE_LIBCRYPTO_MEMCMP)
#    include <openssl/crypto.h>
#  elif defined(HAVE_LIBNETTLE_MEMEQL)
#    include <nettle/memops.h>
#  endif
#endif /* !HAVE_TIMINGSAFE_BCMP && !HAVE_TIMINGSAFE_MEMCMP && !HAVE_CONSTTIME_MEMEQUAL */

#if !defined(HAVE_MEMSET_S) && !defined(HAVE_EXPLICIT_BZERO) && !defined(HAVE_EXPLICIT_MEMSET)
#  if defined(HAVE_LIBSODIUM_MEMZERO)
#    include <sodium/utils.h>
#  elif defined(HAVE_LIBCRYPTO_CLEANSE)
#    include <openssl/crypto.h>
#  endif
#endif /* !HAVE_MEMSET_S && !HAVE_EXPLICIT_BZERO && !HAVE_EXPLICIT_MEMSET */

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
#if defined(HAVE_TIMINGSAFE_BCMP)

	// Oracle Solaris v11.4+, FreeBSD v12.0+, OpenBSD v4.9+, Mac OS v10.12.1+, possibly others
	return timingsafe_bcmp(ptr1, ptr2, len);

#elif defined(HAVE_TIMINGSAFE_MEMCMP)

	// Oracle Solaris v11.4+, FreeBSD v12.0+, OpenBSD v5.6+, possibly others
	return timingsafe_memcmp(ptr1, ptr2, len);

#elif defined(HAVE_CONSTTIME_MEMEQUAL)

	// NetBSD v7.0+, possibly others
	return !consttime_memequal(ptr1, ptr2, len);

#elif defined(HAVE_LIBSODIUM_MEMCMP)

	return sodium_memcmp(ptr1, ptr2, len);

#elif defined(HAVE_LIBCRYPTO_MEMCMP)

	return CRYPTO_memcmp(ptr1, ptr2, len);

#elif defined(HAVE_LIBNETTLE_MEMEQL)

	return !nettle_memeql_sec(ptr1, ptr2, len);

#else

#  warning "No secure library constant-time memory comparison function is available"

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

#endif
}

void
smemzero(void *const restrict ptr, const size_t len)
{
	if (! (ptr && len))
		return;

#if defined(HAVE_MEMSET_S)

	// ISO/IEC 9899:2011 (ISO C11) Annex K 3.7.4.1
	if (memset_s(ptr, len, 0x00, len) != 0)
		RAISE_EXCEPTION;

#elif defined(HAVE_EXPLICIT_BZERO)

	// FreeBSD v11.0+, OpenBSD v5.5+, DragonFlyBSD v5.5+, GNU libc6 v2.25+, musl v1.1.19+, possibly others
	(void) explicit_bzero(ptr, len);

#elif defined(HAVE_EXPLICIT_MEMSET)

	// NetBSD v7.0+, possibly others
	(void) explicit_memset(ptr, 0x00, len);

#elif defined(HAVE_LIBSODIUM_MEMZERO)

	(void) sodium_memzero(ptr, len);

#elif defined(HAVE_LIBCRYPTO_CLEANSE)

	(void) OPENSSL_cleanse(ptr, len);

#else

#  warning "No secure library memory erasing function is available"

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

#endif
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
