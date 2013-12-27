/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 *
 */

#ifndef ATHEME_MEMORY_H
#define ATHEME_MEMORY_H

E void *smalloc(size_t size);
E void *scalloc(size_t elsize, size_t els);
E void *srealloc(void *oldptr, size_t newsize);
E char *sstrdup(const char *s);
E char *sstrndup(const char *s, int len);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
