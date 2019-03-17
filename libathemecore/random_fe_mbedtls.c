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
#include <mbedtls/version.h>

#ifdef HAVE_LIBMBEDCRYPTO_HMAC_DRBG
#  if defined(MBEDTLS_SHA256_C) || defined(MBEDTLS_SHA512_C)
#    include <mbedtls/hmac_drbg.h>
#    include <mbedtls/md.h>
#  else /* MBEDTLS_SHA256_C || MBEDTLS_SHA512_C */
#    error "Neither MBEDTLS_SHA256_C nor MBEDTLS_SHA256_C are defined"
#  endif /* !MBEDTLS_SHA256_C && !MBEDTLS_SHA512_C */
#else /* HAVE_LIBMBEDCRYPTO_HMAC_DRBG */
#  ifdef HAVE_LIBMBEDCRYPTO_CTR_DRBG
#    include <mbedtls/ctr_drbg.h>
#  else /* HAVE_LIBMBEDCRYPTO_CTR_DRBG */
#    error "Neither HAVE_LIBMBEDCRYPTO_CTR_DRBG nor HAVE_LIBMBEDCRYPTO_HMAC_DRBG are defined"
#  endif /* !HAVE_LIBMBEDCRYPTO_CTR_DRBG */
#endif /* !HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

static const char               atheme_drbg_personalisation_string[] = PACKAGE_STRING;
static const void *const        atheme_drbg_const_str = (const void *) atheme_drbg_personalisation_string;
static const size_t             atheme_drbg_const_len = sizeof atheme_drbg_personalisation_string;

static mbedtls_entropy_context  seed_ctx;
static pid_t                    rs_stir_pid = (pid_t) -1;

#ifdef HAVE_LIBMBEDCRYPTO_HMAC_DRBG
static mbedtls_hmac_drbg_context drbg_ctx;
#else /* HAVE_LIBMBEDCRYPTO_HMAC_DRBG */
static mbedtls_ctr_drbg_context drbg_ctx;
#endif /* !HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

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
#ifdef HAVE_LIBMBEDCRYPTO_HMAC_DRBG
		const int ret = mbedtls_hmac_drbg_reseed(&drbg_ctx, atheme_drbg_const_str, atheme_drbg_const_len);
#else /* HAVE_LIBMBEDCRYPTO_HMAC_DRBG */
		const int ret = mbedtls_ctr_drbg_reseed(&drbg_ctx, atheme_drbg_const_str, atheme_drbg_const_len);
#endif /* !HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

		if (ret != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_*_drbg_reseed: error %s", MOWGLI_FUNC_NAME,
			                      atheme_random_mbedtls_strerror(ret));
			exit(EXIT_FAILURE);
		}

		rs_stir_pid = getpid();
	}

#ifdef HAVE_LIBMBEDCRYPTO_HMAC_DRBG
	const int ret = mbedtls_hmac_drbg_random(&drbg_ctx, out, len);
#else /* HAVE_LIBMBEDCRYPTO_HMAC_DRBG */
	const int ret = mbedtls_ctr_drbg_random(&drbg_ctx, out, len);
#endif /* !HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

	if (ret != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_*_drbg_random: error %s", MOWGLI_FUNC_NAME,
		                      atheme_random_mbedtls_strerror(ret));
		exit(EXIT_FAILURE);
	}
}

bool ATHEME_FATTR_WUR
libathemecore_random_early_init(void)
{
	(void) mbedtls_entropy_init(&seed_ctx);

#ifdef HAVE_LIBMBEDCRYPTO_HMAC_DRBG

	(void) mbedtls_hmac_drbg_init(&drbg_ctx);

#ifdef MBEDTLS_SHA512_C
	const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
#else /* MBEDTLS_SHA512_C */
	const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
#endif /* !MBEDTLS_SHA512_C */

	const int ret = mbedtls_hmac_drbg_seed(&drbg_ctx, md_info, &mbedtls_entropy_func, &seed_ctx,
	                                       atheme_drbg_const_str, atheme_drbg_const_len);

#else /* HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

	(void) mbedtls_ctr_drbg_init(&drbg_ctx);

	const int ret = mbedtls_ctr_drbg_seed(&drbg_ctx, &mbedtls_entropy_func, &seed_ctx,
	                                      atheme_drbg_const_str, atheme_drbg_const_len);

#endif /* !HAVE_LIBMBEDCRYPTO_HMAC_DRBG */

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
	char verbuf[64];
	(void) memset(verbuf, 0x00, sizeof verbuf);
	(void) mbedtls_version_get_string(verbuf);

	static char result[1024];
	(void) snprintf(result, sizeof result, "ARM mbedTLS (compiled %s, library %s)", MBEDTLS_VERSION_STRING, verbuf);

	return result;
}
