/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic linker.
 *
 */

#ifndef LINKER_H
#define LINKER_H

extern void *linker_open(const char *path);
extern void *linker_open_ext(const char *path);
extern void *linker_getsym(void *vptr, const char *sym);
extern void linker_close(void *vptr);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
