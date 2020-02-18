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
#include <atheme/bcrypt.h>          // ATHEME_BCRYPT_*
#include <atheme/constants.h>       // BUFSIZE, PASSLEN
#include <atheme/digest.h>          // digest_oneshot_pbkdf2()
#include <atheme/i18n.h>            // _() (gettext)
#include <atheme/libathemecore.h>   // libathemecore_early_init()
#include <atheme/pbkdf2.h>          // PBKDF2_*
#include <atheme/random.h>          // atheme_random_*()
#include <atheme/scrypt.h>          // ATHEME_SCRYPT_*
#include <atheme/stdheaders.h>      // (everything else)
#include <atheme/sysconf.h>         // HAVE_*

#include "benchmark.h"              // (everything else)

#ifdef HAVE_LIBARGON2
#  include <argon2.h>               // argon2_context, argon2_type, argon2_*(), ARGON2_VERSION_NUMBER
#endif

#ifdef HAVE_LIBSODIUM_SCRYPT
#  include <sodium/crypto_pwhash_scryptsalsa208sha256.h> // crypto_pwhash_scryptsalsa208sha256_str()
#endif

static const long double nsec_per_sec = 1000000000.0L;

static unsigned char saltbuf[BUFSIZE];
static unsigned char hashbuf[BUFSIZE];
static char passbuf[PASSLEN + 1];

void ATHEME_FATTR_PRINTF(1, 2)
bench_print(const char *const restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	va_end(ap);
}

bool ATHEME_FATTR_WUR
benchmark_init(void)
{
	if (! libathemecore_early_init())
		// This function logs error messages on failure
		return false;

	(void) atheme_random_buf(saltbuf, BUFSIZE);
	(void) atheme_random_str(passbuf, PASSLEN);
	return true;
}

#ifdef HAVE_ANY_MEMORY_HARD_ALGORITHM

const char *
memory_power2k_to_str(size_t power)
{
	static const char suffixes[] = "KMGT";
	static char result[BUFSIZE];

	size_t memlimit = (1U << power);
	size_t index = 0U;

	while (memlimit >= 1024U && index < 4U)
	{
		memlimit /= 1024U;
		index++;
	}

	(void) snprintf(result, sizeof result, "%zu %ciB", memlimit, suffixes[index]);
	return result;
}

#endif

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

	(void) bench_print("");
	(void) bench_print(_(""
		"%s: '%s' is not a valid type name\n"
		"    valid names are: Argon2d, Argon2i, Argon2id\n"
		"    please see --help\n"
	), MOWGLI_FUNC_NAME, b_name);

	exit(EXIT_FAILURE);
}

void
argon2_print_colheaders(void)
{
	(void) bench_print(_(""
		"\n"
		"Type       MemCost    TimeCost   Threads    Elapsed\n"
		"---------- ---------- ---------- ---------- --------------"
	));
}

void
argon2_print_rowstats(const argon2_type type, const size_t memcost, const size_t timecost, const size_t threads,
                      const long double elapsed)
{
	(void) bench_print(_("%10s %10s %10zu %10zu %13LFs"), argon2_type2string(type, 1),
	                       memory_power2k_to_str(memcost), timecost, threads, elapsed);
}

bool ATHEME_FATTR_WUR
benchmark_argon2(const argon2_type type, const size_t memcost, const size_t timecost, const size_t threadcount,
                 long double *const restrict elapsed)
{
	argon2_context ctx = {
		.out            = hashbuf,
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
		(void) bench_print("argon2_ctx(): %s", argon2_error_message(ret));
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

#ifdef HAVE_LIBSODIUM_SCRYPT

void
scrypt_print_colheaders(void)
{
	(void) bench_print(_(""
		"\n"
		"MemLimit   OpsLimit       Elapsed\n"
		"---------- -------------- --------------"
	));
}

void
scrypt_print_rowstats(const size_t memlimit, const size_t opslimit, const long double elapsed)
{
	(void) bench_print(_("%10s %14zu %13LFs"), memory_power2k_to_str(memlimit), opslimit, elapsed);
}

bool ATHEME_FATTR_WUR
benchmark_scrypt(const size_t memlimit, const size_t opslimit, long double *const restrict elapsed)
{
	const size_t memlimit_real = ((1ULL << memlimit) * 1024ULL);

	struct timespec begin;
	struct timespec end;

	(void) memset(&begin, 0x00, sizeof begin);
	(void) memset(&end, 0x00, sizeof end);

	if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}
	if (crypto_pwhash_scryptsalsa208sha256_str((void *) hashbuf, passbuf, PASSLEN, opslimit, memlimit_real) != 0)
	{
		(void) perror("crypto_pwhash_scryptsalsa208sha256_str(3)");
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

	(void) scrypt_print_rowstats(memlimit, opslimit, duration);
	return true;
}

#endif /* HAVE_LIBSODIUM_SCRYPT */

void
bcrypt_print_colheaders(void)
{
	(void) bench_print(_(""
		"\n"
		"Rounds     Elapsed\n"
		"---------- --------------"
	));
}

void
bcrypt_print_rowstats(const unsigned int rounds, const long double elapsed)
{
	(void) bench_print(_("%10u %13LFs"), rounds, elapsed);
}

bool ATHEME_FATTR_WUR
benchmark_bcrypt(const unsigned int rounds, long double *const restrict elapsed)
{
	struct timespec begin;
	struct timespec end;

	(void) memset(&begin, 0x00, sizeof begin);
	(void) memset(&end, 0x00, sizeof end);

	if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}
	if (! atheme_eks_bf_compute(passbuf, ATHEME_BCRYPT_VERSION_MINOR, rounds, saltbuf, hashbuf))
	{
		(void) bench_print("atheme_bcrypt_compute() failed");
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

	(void) bcrypt_print_rowstats(rounds, duration);
	return true;
}

const char *
md_digest_to_name(const enum digest_algorithm b_digest, const bool with_sasl_scram)
{
	if (with_sasl_scram)
	{
		switch (b_digest)
		{
			case DIGALG_MD5:
				return "SCRAM-MD5";
			case DIGALG_SHA1:
				return "SCRAM-SHA-1";
			case DIGALG_SHA2_256:
				return "SCRAM-SHA-256";
			case DIGALG_SHA2_512:
				return "SCRAM-SHA-512";
		}
	}
	else
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

	(void) bench_print("");
	(void) bench_print(_(""
		"%s: '%s' is not a valid algorithm name\n"
		"    valid names are: MD5, SHA1, SHA2-256, SHA2-512\n"
		"    please see --help\n"
	), MOWGLI_FUNC_NAME, b_name);

	exit(EXIT_FAILURE);
}

void
pbkdf2_print_colheaders(void)
{
	(void) bench_print(_(""
		"\n"
		"Digest           Iterations     Elapsed\n"
		"---------------- -------------- --------------"
	));
}

void
pbkdf2_print_rowstats(const enum digest_algorithm digest, const size_t iterations, const bool with_sasl_scram,
                      const long double elapsed)
{
	(void) bench_print(_("%16s %14zu %13LFs"), md_digest_to_name(digest, with_sasl_scram), iterations, elapsed);
}

bool ATHEME_FATTR_WUR
benchmark_pbkdf2(const enum digest_algorithm digest, const size_t itercount, const bool with_sasl_scram,
                 long double *const restrict elapsed)
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
		(void) bench_print("digest_oneshot_pbkdf2() failed");
		return false;
	}
	if (with_sasl_scram)
	{
		static const char ServerKeyConstant[] = "Server Key";
		static const char ClientKeyConstant[] = "Client Key";
		unsigned char ServerKey[DIGEST_MDLEN_MAX];
		unsigned char ClientKey[DIGEST_MDLEN_MAX];
		unsigned char StoredKey[DIGEST_MDLEN_MAX];

		if (! digest_oneshot_hmac(digest, hashbuf, mdlen, ServerKeyConstant, 10U, ServerKey, NULL))
		{
			(void) bench_print("digest_oneshot_hmac(ServerKey) failed");
			return false;
		}
		if (! digest_oneshot_hmac(digest, hashbuf, mdlen, ClientKeyConstant, 10U, ClientKey, NULL))
		{
			(void) bench_print("digest_oneshot_hmac(ClientKey) failed");
			return false;
		}
		if (! digest_oneshot(digest, ClientKey, mdlen, StoredKey, NULL))
		{
			(void) bench_print("digest_oneshot() failed");
			return false;
		}
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

	(void) pbkdf2_print_rowstats(digest, itercount, with_sasl_scram, duration);
	return true;
}
