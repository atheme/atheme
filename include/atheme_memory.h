/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 */

#ifndef ATHEME_MEMORY_H
#define ATHEME_MEMORY_H

extern void *smalloc(size_t len);
extern void *scalloc(size_t num, size_t len);
extern void *srealloc(void *ptr, size_t len);
extern char *sstrdup(const char *ptr);
extern char *sstrndup(const char *ptr, size_t len);

#endif
