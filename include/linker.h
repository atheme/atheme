/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic linker.
 */

#ifndef ATHEME_INC_LINKER_H
#define ATHEME_INC_LINKER_H 1

mowgli_module_t *linker_open_ext(const char *path, char *errbuf, int errlen);

#endif /* !ATHEME_INC_LINKER_H */
