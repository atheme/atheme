/*
 * Frontend routines for the random interface (ARM mbedTLS backend).
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
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

#include "atheme.h"

#include <mbedtls/entropy.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/md.h>

#if defined(MBEDTLS_ERROR_C) && defined(HAVE_MBEDTLS_ERROR_H)
#  include <mbedtls/error.h>
#endif

#if defined(MBEDTLS_VERSION_C) && defined(HAVE_MBEDTLS_VERSION_H)
#  include <mbedtls/version.h>
#endif

static const char atheme_pers_str[] = PACKAGE_STRING;

static mbedtls_entropy_context seed_ctx;
static mbedtls_hmac_drbg_context hmac_ctx;

static pid_t rs_stir_pid = (pid_t) -1;

static const char *
atheme_random_mbedtls_strerror(int err)
{
	static char errbuf[384];

	if (err < 0)
		err = -err;

#if defined(MBEDTLS_ERROR_C) && defined(HAVE_MBEDTLS_ERROR_H)
	char mbed_errbuf[320];

	(void) mbedtls_strerror(err, mbed_errbuf, sizeof mbed_errbuf);
	(void) snprintf(errbuf, sizeof errbuf, "-0x%X: %s", (unsigned int) err, mbed_errbuf);
#else
	(void) snprintf(errbuf, sizeof errbuf, "-0x%X", (unsigned int) err);
#endif

	return errbuf;
}

uint32_t
atheme_random(void)
{
	uint32_t val;

	(void) atheme_random_buf(&val, sizeof val);

	return val;
}

uint32_t
atheme_random_uniform(const uint32_t bound)
{
	if (bound < 2)
		return 0;

	const uint32_t min = -bound % bound;

	for (;;)
	{
		uint32_t candidate;

		(void) atheme_random_buf(&candidate, sizeof candidate);

		if (candidate >= min)
			return candidate % bound;
	}
}

void
atheme_random_buf(void *const restrict out, const size_t len)
{
	if (rs_stir_pid == (pid_t) -1)
	{
		const mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
		const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(md_type);

		(void) mbedtls_entropy_init(&seed_ctx);
		(void) mbedtls_hmac_drbg_init(&hmac_ctx);

		const int ret = mbedtls_hmac_drbg_seed(&hmac_ctx, md_info, &mbedtls_entropy_func, &seed_ctx,
		                                       (const void *) atheme_pers_str, sizeof atheme_pers_str);

		if (ret != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_seed: error %s", __func__,
			                      atheme_random_mbedtls_strerror(ret));
			exit(EXIT_FAILURE);
		}

		rs_stir_pid = getpid();
	}
	else if (getpid() != rs_stir_pid)
	{
		const int ret = mbedtls_hmac_drbg_reseed(&hmac_ctx, NULL, 0);

		if (ret != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_reseed: error %s", __func__,
			                      atheme_random_mbedtls_strerror(ret));
			exit(EXIT_FAILURE);
		}

		rs_stir_pid = getpid();
	}

	const int ret = mbedtls_hmac_drbg_random(&hmac_ctx, out, len);

	if (ret != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_random: error %s", __func__,
		                      atheme_random_mbedtls_strerror(ret));
		exit(EXIT_FAILURE);
	}
}

const char *
random_get_frontend_info(void)
{
#if defined(MBEDTLS_VERSION_C) && defined(HAVE_MBEDTLS_VERSION_H)
	char verbuf[64];
	(void) memset(verbuf, 0x00, sizeof verbuf);
	(void) mbedtls_version_get_string(verbuf);

	static char result[1024];
	(void) snprintf(result, sizeof result, "ARM mbedTLS (compiled %s, library %s)", MBEDTLS_VERSION_STRING, verbuf);

	return result;
#else
	return "ARM mbedTLS (unknown version)";
#endif
}
