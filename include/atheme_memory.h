/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 *
 * $Id: atheme_memory.h 7771 2007-03-03 12:46:36Z pippijn $
 */

#ifndef __CLAROBASEMEMORY
#define __CLAROBASEMEMORY

E void *smalloc(size_t size);
E void *scalloc(size_t elsize, size_t els);
E void *srealloc(void *oldptr, size_t newsize);
E char *sstrdup(const char *s);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
