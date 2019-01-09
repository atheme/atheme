/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the random interface (libsodium backend).
 */

#ifndef ATHEME_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_RANDOM_FRONTEND_C */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "atheme_random.h"

#include <sodium/core.h>
#include <sodium/randombytes.h>
#include <sodium/version.h>

uint32_t
atheme_random(void)
{
	return randombytes_random();
}

uint32_t
atheme_random_uniform(const uint32_t bound)
{
	return randombytes_uniform(bound);
}

void
atheme_random_buf(void *const restrict out, const size_t len)
{
	(void) randombytes_buf(out, len);
}

const char *
random_get_frontend_info(void)
{
	static char result[1024];
	(void) snprintf(result, sizeof result, "libsodium (compiled %s, library %s)",
	                SODIUM_VERSION_STRING, sodium_version_string());

	return result;
}
