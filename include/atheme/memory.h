/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Memory stuff.
 */

#ifndef ATHEME_INC_MEMORY_H
#define ATHEME_INC_MEMORY_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

#if !defined(HAVE_MEMSET_S) && !defined(HAVE_EXPLICIT_BZERO) && !defined(HAVE_LIBSODIUM_MEMZERO)
// This symbol is located in libathemecore/atheme.c
extern void *(* volatile volatile_memset)(void *, int, size_t);
#endif /* !HAVE_MEMSET_S && !HAVE_EXPLICIT_BZERO && !HAVE_LIBSODIUM_MEMZERO */



int smemcmp(const void *ptr1, const void *ptr2, size_t len)
    ATHEME_FATTR_WUR
    ATHEME_FATTR_DIAGNOSE_IF(!ptr1, "calling smemcmp() with !ptr1", "error")
    ATHEME_FATTR_DIAGNOSE_IF(!ptr2, "calling smemcmp() with !ptr2", "error")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smemcmp() with !len", "error");

void smemzerofree(void *ptr, size_t len)
    ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling smemzerofree() with !ptr", "warning")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smemzerofree() with !len", "warning");

void smemzero(void *ptr, size_t len)
    ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling smemzero() with !ptr", "warning")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smemzero() with !len", "error");

void sfree(void *ptr);

void *scalloc(size_t num, size_t len)
    ATHEME_FATTR_ALLOC_SIZE_PRODUCT(1, 2)
    ATHEME_FATTR_MALLOC
    ATHEME_FATTR_RETURNS_NONNULL
    ATHEME_FATTR_DIAGNOSE_IF(!num, "calling scalloc() with !num", "error")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling scalloc() with !len", "error");

void *smalloc(size_t len)
    ATHEME_FATTR_ALLOC_SIZE(1)
    ATHEME_FATTR_MALLOC
    ATHEME_FATTR_RETURNS_NONNULL
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smalloc() with !len", "error");

void *srealloc(void *ptr, size_t len)
    ATHEME_FATTR_ALLOC_SIZE(2)
    ATHEME_FATTR_WUR
    ATHEME_FATTR_DIAGNOSE_IF((!ptr && !len), "calling srealloc() with (!ptr && !len)", "warning");

void *sreallocarray(void *ptr, size_t num, size_t len)
    ATHEME_FATTR_ALLOC_SIZE_PRODUCT(2, 3)
    ATHEME_FATTR_WUR
    ATHEME_FATTR_DIAGNOSE_IF((!ptr && !num), "calling sreallocarray() with (!ptr && !num)", "warning")
    ATHEME_FATTR_DIAGNOSE_IF((!ptr && !len), "calling sreallocarray() with (!ptr && !len)", "warning");

char *sstrdup(const char *ptr)
    ATHEME_FATTR_MALLOC
    ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling sstrdup() with !ptr", "warning");

char *sstrndup(const char *ptr, size_t maxlen)
    ATHEME_FATTR_MALLOC
    ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling sstrndup() with !ptr", "warning")
    ATHEME_FATTR_DIAGNOSE_IF(!maxlen, "calling sstrndup() with !maxlen", "error");

void *smemdup(const void *ptr, size_t len)
    ATHEME_FATTR_MALLOC
    ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling smemdup() with !ptr", "warning")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smemdup() with !len", "error");

#endif /* !ATHEME_INC_MEMORY_H */
