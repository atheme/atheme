/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 */

#ifndef ATHEME_INC_ATHEME_MEMORY_H
#define ATHEME_INC_ATHEME_MEMORY_H 1

#include <stddef.h>

#include "attrs.h"
#include "sysconf.h"

#if !defined(HAVE_MEMSET_S) && !defined(HAVE_EXPLICIT_BZERO) && !defined(HAVE_LIBSODIUM_MEMZERO)
// This symbol is located in libathemecore/atheme.c
extern void *(* volatile volatile_memset)(void *, int, size_t);
#endif /* !HAVE_MEMSET_S && !HAVE_EXPLICIT_BZERO && !HAVE_LIBSODIUM_MEMZERO */

void smemzero(void *ptr, size_t len);
void sfree(void *ptr);
void *scalloc(size_t num, size_t len) ATHEME_FATTR_ALLOC_SIZE_PRODUCT(1, 2) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL;
void *smalloc(size_t len) ATHEME_FATTR_ALLOC_SIZE(1) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL;
void *srealloc(void *ptr, size_t len) ATHEME_FATTR_ALLOC_SIZE(2) ATHEME_FATTR_WUR;
char *sstrdup(const char *ptr) ATHEME_FATTR_MALLOC;
char *sstrndup(const char *ptr, size_t maxlen) ATHEME_FATTR_MALLOC;

#ifdef ATHEME_ATTR_HAS_DIAGNOSE_IF
void smemzero(void *ptr, size_t len) ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling smemzero() with !ptr", "warning");
void smemzero(void *ptr, size_t len) ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smemzero() with !len", "warning");
void sfree(void *ptr) ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling sfree() with !ptr", "warning");
void *scalloc(size_t num, size_t len) ATHEME_FATTR_DIAGNOSE_IF(!num, "calling scalloc() with !num", "error");
void *scalloc(size_t num, size_t len) ATHEME_FATTR_DIAGNOSE_IF(!len, "calling scalloc() with !len", "error");
void *smalloc(size_t len) ATHEME_FATTR_DIAGNOSE_IF(!len, "calling smalloc() with !len", "error");
void *realloc(void *ptr, size_t len) ATHEME_FATTR_DIAGNOSE_IF(!ptr && !len, "calling srealloc() with !ptr && !len", "warning");
char *sstrdup(const char *ptr) ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling sstrdup() with !ptr", "warning");
char *sstrndup(const char *ptr, size_t maxlen) ATHEME_FATTR_DIAGNOSE_IF(!ptr, "calling sstrndup() with !ptr", "warning");
char *sstrndup(const char *ptr, size_t maxlen) ATHEME_FATTR_DIAGNOSE_IF(!maxlen, "calling sstrndup() with !len", "error");
#endif /* ATHEME_ATTR_HAS_DIAGNOSE_IF */

#endif /* !ATHEME_INC_ATHEME_MEMORY_H */
