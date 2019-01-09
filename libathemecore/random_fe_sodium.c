/*
 * Frontend routines for the random interface (libsodium backend).
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
