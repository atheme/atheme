/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Atheme IRC Services random number generator interface.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_RANDOM_H
#define ATHEME_INC_RANDOM_H 1

#include "attrs.h"

uint32_t atheme_random(void);
uint32_t atheme_random_uniform(uint32_t bound);
void atheme_random_buf(void *buf, size_t len);

bool libathemecore_random_early_init(void) ATHEME_FATTR_WUR;
const char *random_get_frontend_info(void);

#ifdef ATHEME_ATTR_HAS_DIAGNOSE_IF
uint32_t atheme_random_uniform(uint32_t bound)
    ATHEME_FATTR_DIAGNOSE_IF((bound < 2), "calling atheme_random_uniform() with bound < 2", "error");
void atheme_random_buf(void *buf, size_t len)
    ATHEME_FATTR_DIAGNOSE_IF(!buf, "calling atheme_random_buf() with !buf", "error");
void atheme_random_buf(void *buf, size_t len)
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling atheme_random_buf() with !len", "error");
#endif /* ATHEME_ATTR_HAS_DIAGNOSE_IF */

#endif /* !ATHEME_INC_RANDOM_H */
