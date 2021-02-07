/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod@nenolod.net>
 *
 * Dynamic linker.
 */

#ifndef ATHEME_INC_LINKER_H
#define ATHEME_INC_LINKER_H 1

#include <atheme/stdheaders.h>

mowgli_module_t *linker_open_ext(const char *path, char *errbuf, size_t errlen);

#endif /* !ATHEME_INC_LINKER_H */
