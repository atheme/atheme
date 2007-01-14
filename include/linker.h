/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic linker.
 *
 * $Id: linker.h 7467 2007-01-14 03:25:42Z nenolod $
 */

#ifndef LINKER_H
#define LINKER_H

extern void *linker_open(char *path);
extern void *linker_open_ext(char *path);
extern void *linker_getsym(void *vptr, char *sym);
extern void linker_close(void *vptr);

#endif
