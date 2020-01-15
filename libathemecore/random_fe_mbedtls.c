/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the random interface (ARM mbedTLS backend).
 */

#ifndef ATHEME_LAC_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_LAC_RANDOM_FRONTEND_C */

#ifdef MBEDTLS_CONFIG_FILE
#  include MBEDTLS_CONFIG_FILE
#else
#  include <mbedtls/config.h>
#endif

#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/md.h>
#include <mbedtls/version.h>

static const char atheme_drbg_const_str[] = PACKAGE_STRING;
static const size_t atheme_drbg_const_len = sizeof atheme_drbg_const_str;

static mbedtls_entropy_context seed_ctx;
static mbedtls_hmac_drbg_context drbg_ctx;

static pid_t rs_stir_pid = (pid_t) -1;

static const char *
atheme_random_mbedtls_strerror(const int err)
{
	char mbed_errbuf[320];
	(void) mbedtls_strerror(err, mbed_errbuf, sizeof mbed_errbuf);

	static char errbuf[384];
	(void) snprintf(errbuf, sizeof errbuf, "-0x%X: %s", (unsigned int) -err, mbed_errbuf);

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
	if (rs_stir_pid == -1)
	{
		(void) slog(LG_ERROR, "%s: called before early init (BUG)", MOWGLI_FUNC_NAME);
		abort();
	}

	if (rs_stir_pid != getpid())
	{
		const int ret = mbedtls_hmac_drbg_reseed(&drbg_ctx, (const void *) atheme_drbg_const_str,
		                                         atheme_drbg_const_len);

		if (ret != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_reseed(3): error %s", MOWGLI_FUNC_NAME,
			                      atheme_random_mbedtls_strerror(ret));
			exit(EXIT_FAILURE);
		}

		rs_stir_pid = getpid();
	}

	const int ret = mbedtls_hmac_drbg_random(&drbg_ctx, out, len);

	if (ret != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_hmac_drbg_random(3): error %s", MOWGLI_FUNC_NAME,
		                      atheme_random_mbedtls_strerror(ret));
		exit(EXIT_FAILURE);
	}
}

bool ATHEME_FATTR_WUR
libathemecore_random_early_init(void)
{
	(void) mbedtls_entropy_init(&seed_ctx);
	(void) mbedtls_hmac_drbg_init(&drbg_ctx);

#ifdef MBEDTLS_SHA512_C
	const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
#else /* MBEDTLS_SHA512_C */
	const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
#endif /* !MBEDTLS_SHA512_C */

	const int ret = mbedtls_hmac_drbg_seed(&drbg_ctx, md_info, &mbedtls_entropy_func, &seed_ctx,
	                                       (const void *) atheme_drbg_const_str, atheme_drbg_const_len);

	if (ret != 0)
	{
		(void) fprintf(stderr, "ARM mbedTLS: Early RNG initialization failed!\n");
		(void) fprintf(stderr, "ARM mbedTLS: Error %s\n", atheme_random_mbedtls_strerror(ret));
		return false;
	}

	rs_stir_pid = getpid();

	return true;
}

const char *
random_get_frontend_info(void)
{
	char verbuf[BUFSIZE];
	(void) memset(verbuf, 0x00, sizeof verbuf);
	(void) mbedtls_version_get_string(verbuf);

#ifdef MBEDTLS_SHA512_C
	const char hmac_alg[] = "SHA2-512";
#else /* MBEDTLS_SHA512_C */
	const char hmac_alg[] = "SHA2-256";
#endif /* !MBEDTLS_SHA512_C */

	static char result[BUFSIZE];
	(void) snprintf(result, sizeof result, "ARM mbedTLS HMAC-%s-DRBG (compiled %s, library %s)",
	                                       hmac_alg, MBEDTLS_VERSION_STRING, verbuf);

	return result;
}
