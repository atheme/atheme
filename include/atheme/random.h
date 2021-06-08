/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Atheme IRC Services random number generator interface.
 */

#ifndef ATHEME_INC_RANDOM_H
#define ATHEME_INC_RANDOM_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

uint32_t atheme_random(void);

uint32_t atheme_random_uniform(uint32_t bound)
    ATHEME_FATTR_DIAGNOSE_IF((bound < 2), "calling atheme_random_uniform() with bound < 2", "error");

void atheme_random_buf(void *buf, size_t len)
    ATHEME_FATTR_DIAGNOSE_IF(!buf, "calling atheme_random_buf() with !buf", "error")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling atheme_random_buf() with !len", "error");

void atheme_random_str(char *buf, size_t len)
    ATHEME_FATTR_DIAGNOSE_IF(!buf, "calling atheme_random_str() with !buf", "error")
    ATHEME_FATTR_DIAGNOSE_IF(!len, "calling atheme_random_str() with !len", "error");

bool libathemecore_random_early_init(void) ATHEME_FATTR_WUR;
const char *random_get_frontend_info(void);

#endif /* !ATHEME_INC_RANDOM_H */
