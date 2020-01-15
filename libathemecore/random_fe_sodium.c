/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the random interface (libsodium backend).
 */

#ifndef ATHEME_LAC_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_LAC_RANDOM_FRONTEND_C */

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

bool ATHEME_FATTR_WUR
libathemecore_random_early_init(void)
{
	(void) atheme_random();

	return true;
}

const char *
random_get_frontend_info(void)
{
	static char result[BUFSIZE];
	(void) snprintf(result, sizeof result, "libsodium (compiled %s, library %s)",
	                SODIUM_VERSION_STRING, sodium_version_string());

	return result;
}
