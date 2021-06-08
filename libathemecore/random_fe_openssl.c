/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Frontend routines for the random interface (OpenSSL backend).
 */

#ifndef ATHEME_LAC_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_LAC_RANDOM_FRONTEND_C */

#include <openssl/err.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>

static bool rng_init_done = false;

static void
atheme_openssl_clear_errors(void)
{
	// Just call ERR_get_error() to pop errors off the OpenSSL error stack until it returns 0 (no more errors)
	for (/* No initialization */; ERR_get_error() != 0; /* No action */) { }
}

static const char *
atheme_openssl_get_strerror(void)
{
	static char res[BUFSIZE];
	const char *efile = NULL;
	const char *edata = NULL;
	int eline = 0;
	int eflags = 0;

	const unsigned long err = ERR_get_error_line_data(&efile, &eline, &edata, &eflags);

	if (! err)
		return "<unknown>";

	if (! efile)
		efile = "<unknown>";

	if ((eflags & ERR_TXT_STRING) && edata)
		(void) snprintf(res, sizeof res, "%08lX (%s) [%s:%d]", err, edata, efile, eline);
	else
		(void) snprintf(res, sizeof res, "%08lX (<unknown>) [%s:%d]", err, efile, eline);

	return res;
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
	if (! rng_init_done)
		abort();

	(void) atheme_openssl_clear_errors();

	if (RAND_bytes(out, (const int) len) != 1)
	{
		(void) slog(LG_ERROR, "%s: RAND_bytes(3): %s", MOWGLI_FUNC_NAME, atheme_openssl_get_strerror());
		abort();
	}
}

bool ATHEME_FATTR_WUR
libathemecore_random_early_init(void)
{
	(void) atheme_openssl_clear_errors();

	if (RAND_status() != 1)
	{
		(void) fprintf(stderr, "OpenSSL: RNG initialization failed!\n");
		(void) fprintf(stderr, "OpenSSL: Error %s\n", atheme_openssl_get_strerror());
		return false;
	}

	/* Add some data to personalise the RNG. This does not contribute any
	 * entropy (third argument), and we make sure to do it only after the
	 * RNG has already been initialized, as tested above.   -- amdj
	 */
	(void) RAND_add(PACKAGE_STRING, (int) strlen(PACKAGE_STRING), (double) 0);

	rng_init_done = true;
	return true;
}

const char *
random_get_frontend_info(void)
{
	return OPENSSL_VERSION_TEXT;
}
