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
#include <atheme/argon2.h>          // ATHEME_ARGON2_*
#include <atheme/constants.h>       // BUFSIZE, PASSLEN
#include <atheme/digest.h>          // digest_oneshot_pbkdf2()
#include <atheme/libathemecore.h>   // libathemecore_early_init()
#include <atheme/pbkdf2.h>          // PBKDF2_*
#include <atheme/random.h>          // atheme_random_*()
#include <atheme/stdheaders.h>      // (everything else)
#include <atheme/sysconf.h>         // HAVE_LIBARGON2

#include "benchmark.h"              // (everything else)

#ifdef HAVE_LIBARGON2
#  include <argon2.h>               // argon2_context, argon2_type, argon2_*(), ARGON2_VERSION_NUMBER
#endif

static const long double nsec_per_sec = 1000000000.0L;

static unsigned char saltbuf[BUFSIZE];
static char hashbuf[BUFSIZE];
static char passbuf[BUFSIZE];

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

argon2_type
argon2_name_to_type(const char *const restrict b_name)
{
	if (strcasecmp(b_name, "Argon2d") == 0)
		return Argon2_d;
	else if (strcasecmp(b_name, "Argon2i") == 0)
		return Argon2_i;
	else if (strcasecmp(b_name, "Argon2id") == 0)
		return Argon2_id;

	(void) fprintf(stderr, "%s: '%s' is not a valid type name\n", MOWGLI_FUNC_NAME, b_name);
	(void) fprintf(stderr, "%s: valid names are: Argon2d, Argon2i, Argon2id\n", MOWGLI_FUNC_NAME);

	exit(EXIT_FAILURE);
}

void
argon2_print_colheaders(void)
{
	(void) fprintf(stderr, "%-10s %-14s %-10s %-10s %-14s\n",
	                       "Type", "MemCost", "TimeCost", "Threads", "Elapsed");
	(void) fprintf(stderr, "---------- -------------- ---------- ---------- --------------\n");
}

void
argon2_print_rowstats(const argon2_type type, const size_t memcost, const size_t timecost, const size_t threads,
                      const long double elapsed)
{
	(void) fprintf(stderr, "%10s %13uK %10zu %10zu %13LFs\n", argon2_type2string(type, 1),
	                       (1U << memcost), timecost, threads, elapsed);
}

bool ATHEME_FATTR_WUR
benchmark_argon2(const argon2_type type, const size_t memcost, const size_t timecost, const size_t threadcount,
                 long double *const restrict elapsed)
{
	argon2_context ctx = {
		.out            = (void *) hashbuf,
		.outlen         = ATHEME_ARGON2_HASHLEN_DEF,
		.pwd            = (void *) passbuf,
		.pwdlen         = PASSLEN,
		.salt           = saltbuf,
		.saltlen        = ATHEME_ARGON2_SALTLEN_DEF,
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
	const long double duration = (end_ld - begin_ld);

	if (elapsed)
		*elapsed = duration;

	(void) argon2_print_rowstats(type, memcost, timecost, threadcount, duration);
	return true;
}

#endif /* HAVE_LIBARGON2 */

const char *
md_digest_to_name(const enum digest_algorithm b_digest)
{
	switch (b_digest)
	{
		case DIGALG_MD5:
			return "MD5";
		case DIGALG_SHA1:
			return "SHA1";
		case DIGALG_SHA2_256:
			return "SHA256";
		case DIGALG_SHA2_512:
			return "SHA512";
	}

	return NULL;
}

enum digest_algorithm
md_name_to_digest(const char *const restrict b_name)
{
	if (strcasecmp(b_name, "MD5") == 0)
		return DIGALG_MD5;
	else if (strcasecmp(b_name, "SHA1") == 0)
		return DIGALG_SHA1;
	else if (strcasecmp(b_name, "SHA2-256") == 0)
		return DIGALG_SHA2_256;
	else if (strcasecmp(b_name, "SHA2-512") == 0)
		return DIGALG_SHA2_512;

	(void) fprintf(stderr, "%s: '%s' is not a valid algorithm name\n", MOWGLI_FUNC_NAME, b_name);
	(void) fprintf(stderr, "%s: valid names are: MD5, SHA1, SHA2-256, SHA2-512\n", MOWGLI_FUNC_NAME);

	exit(EXIT_FAILURE);
}

void
pbkdf2_print_colheaders(void)
{
	(void) fprintf(stderr, "%-10s %-10s %-14s\n", "Digest", "Iterations", "Elapsed");
	(void) fprintf(stderr, "---------- ---------- --------------\n");
}

void
pbkdf2_print_rowstats(const enum digest_algorithm digest, const size_t iterations, const long double elapsed)
{
	(void) fprintf(stderr, "%10s %10zu %13LFs\n", md_digest_to_name(digest), iterations, elapsed);
}

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
	const long double duration = (end_ld - begin_ld);

	if (elapsed)
		*elapsed = duration;

	(void) pbkdf2_print_rowstats(digest, itercount, duration);
	return true;
}
