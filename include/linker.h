/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic linker.
 */

#ifndef LINKER_H
#define LINKER_H

mowgli_module_t *linker_open_ext(const char *path, char *errbuf, int errlen);

#endif
