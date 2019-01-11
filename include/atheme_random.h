/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Atheme IRC Services random number generator interface.
 */

#include "sysconf.h"

#ifndef ATHEME_INC_RANDOM_H
#define ATHEME_INC_RANDOM_H 1

#include <stddef.h>
#include <stdint.h>

uint32_t atheme_random(void);
uint32_t atheme_random_uniform(uint32_t);
void atheme_random_buf(void *, size_t);

const char *random_get_frontend_info(void);

#endif /* !ATHEME_INC_RANDOM_H */
