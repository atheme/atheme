/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme/attributes.h>      // ATHEME_FATTR_WUR
#include <atheme/argon2.h>          // ARGON2_*
#include <atheme/constants.h>       // BUFSIZE, PASSLEN
#include <atheme/digest.h>          // digest_oneshot_pbkdf2()
#include <atheme/libathemecore.h>   // libathemecore_early_init()
#include <atheme/pbkdf2.h>          // PBKDF2_*
#include <atheme/random.h>          // atheme_random_*()
#include <atheme/stdheaders.h>      // (everything else)
#include <atheme/sysconf.h>         // HAVE_LIBARGON2

#include "benchmark.h"              // self-declarations
#include "utils.h"                  // (everything else)

static const long double nsec_per_sec = 1000000000.0L;

static unsigned char hashbuf[BUFSIZE];
static unsigned char saltbuf[BUFSIZE];
static char passbuf[PASSLEN + 1];

bool ATHEME_FATTR_WUR
benchmark_init(void)
{
	if (! libathemecore_early_init())
		return false;

	(void) atheme_random_buf(saltbuf, BUFSIZE);
	(void) atheme_random_str(passbuf, PASSLEN);
	return true;
}

#ifdef HAVE_LIBARGON2

bool ATHEME_FATTR_WUR
benchmark_argon2(const argon2_type type, const size_t memcost, const size_t timecost, const size_t threadcount,
                 long double *const restrict elapsed)
{
	argon2_context ctx = {
		.out            = hashbuf,
		.outlen         = ARGON2_HASHLEN_DEF,
		.pwd            = (void *) passbuf,
		.pwdlen         = PASSLEN,
		.salt           = saltbuf,
		.saltlen        = ARGON2_SALTLEN_DEF,
		.t_cost         = timecost,
		.m_cost         = (1U << memcost),
		.lanes          = threadcount,
		.threads        = threadcount,
		.version        = ARGON2_VERSION_NUMBER,
	};

	struct timespec begin;
	struct timespec end;
	int ret;

	(void) memset(&begin, 0x00, sizeof begin);
	(void) memset(&end, 0x00, sizeof end);

	if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}
	if ((ret = argon2_ctx(&ctx, type)) != (int) ARGON2_OK)
	{
		(void) fprintf(stderr, "argon2_ctx() failed: %s\n", argon2_error_message(ret));
		return false;
	}
	if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}

	const long double begin_ld = ((long double) begin.tv_sec) + (((long double) begin.tv_nsec) / nsec_per_sec);
	const long double end_ld = ((long double) end.tv_sec) + (((long double) end.tv_nsec) / nsec_per_sec);

	*elapsed = (end_ld - begin_ld);

	(void) argon2_print_rowstats(type, memcost, timecost, threadcount, *elapsed);
	return true;
}

#endif /* HAVE_LIBARGON2 */

bool ATHEME_FATTR_WUR
benchmark_pbkdf2(const enum digest_algorithm digest, const size_t itercount, long double *const restrict elapsed)
{
	const size_t mdlen = digest_size_alg(digest);
	struct timespec begin;
	struct timespec end;

	(void) memset(&begin, 0x00, sizeof begin);
	(void) memset(&end, 0x00, sizeof end);

	if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}
	if (! digest_oneshot_pbkdf2(digest, passbuf, PASSLEN, saltbuf, PBKDF2_SALTLEN_DEF, itercount, hashbuf, mdlen))
	{
		(void) fprintf(stderr, "digest_oneshot_pbkdf2() failed\n");
		return false;
	}
	if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}

	const long double begin_ld = ((long double) begin.tv_sec) + (((long double) begin.tv_nsec) / nsec_per_sec);
	const long double end_ld = ((long double) end.tv_sec) + (((long double) end.tv_nsec) / nsec_per_sec);

	*elapsed = (end_ld - begin_ld);

	(void) pbkdf2_print_rowstats(digest, itercount, *elapsed);
	return true;
}
