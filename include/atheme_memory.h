/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 */

#ifndef ATHEME_INC_ATHEME_MEMORY_H
#define ATHEME_INC_ATHEME_MEMORY_H 1

#include "sysconf.h"

#if !defined(HAVE_MEMSET_S) && !defined(HAVE_EXPLICIT_BZERO) && !defined(HAVE_LIBSODIUM)
// atheme.c
extern void *(* volatile volatile_memset)(void *, int, size_t);
#endif /* !HAVE_MEMSET_S && !HAVE_EXPLICIT_BZERO && !HAVE_LIBSODIUM */

void smemzero(void *p, size_t n);
void sfree(void *ptr);
void *scalloc(size_t num, size_t len) ATHEME_FATTR_MALLOC;
void *smalloc(size_t len) ATHEME_FATTR_MALLOC;
void *srealloc(void *ptr, size_t len);
char *sstrdup(const char *ptr) ATHEME_FATTR_MALLOC;
char *sstrndup(const char *ptr, size_t len) ATHEME_FATTR_MALLOC;

#endif /* !ATHEME_INC_ATHEME_MEMORY_H */
