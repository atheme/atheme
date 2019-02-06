/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock <nenolod@nenolod.net>
 *
 * Dynamic linker.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_LINKER_H
#define ATHEME_INC_LINKER_H 1

mowgli_module_t *linker_open_ext(const char *path, char *errbuf, int errlen);

#endif /* !ATHEME_INC_LINKER_H */
