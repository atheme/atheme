/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 *
 * $Id: atheme_memory.h 7467 2007-01-14 03:25:42Z nenolod $
 */

#ifndef __CLAROBASEMEMORY
#define __CLAROBASEMEMORY

E void *smalloc(size_t size);
E void *scalloc(size_t elsize, size_t els);
E void *srealloc(void *oldptr, size_t newsize);
E char *sstrdup(const char *s);

#endif
