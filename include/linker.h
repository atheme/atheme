/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic linker.
 *
 * $Id: linker.h 7771 2007-03-03 12:46:36Z pippijn $
 */

#ifndef LINKER_H
#define LINKER_H

extern void *linker_open(char *path);
extern void *linker_open_ext(char *path);
extern void *linker_getsym(void *vptr, char *sym);
extern void linker_close(void *vptr);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
