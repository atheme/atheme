/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 */

#ifndef ATHEME_MEMORY_H
#define ATHEME_MEMORY_H

extern void *smalloc(size_t size);
extern void *scalloc(size_t elsize, size_t els);
extern void *srealloc(void *oldptr, size_t newsize);
extern char *sstrdup(const char *s);
extern char *sstrndup(const char *s, size_t len);

#endif
