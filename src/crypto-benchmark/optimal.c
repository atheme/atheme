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

#include <atheme/attributes.h>  // ATHEME_FATTR_WUR
#include <atheme/argon2.h>      // ARGON2_*
#include <atheme/digest.h>      // DIGALG_*, digest_oneshot_pbkdf2()
#include <atheme/pbkdf2.h>      // PBKDF2_*
#include <atheme/stdheaders.h>  // (everything else)
#include <atheme/sysconf.h>     // HAVE_LIBARGON2

#include "benchmark.h"          // benchmark_*()
#include "optimal.h"            // self-declarations
#include "utils.h"              // (everything else)

#ifdef HAVE_LIBARGON2

static bool ATHEME_FATTR_WUR
do_optimal_argon2_benchmark(const long double clocklimit, const size_t memlimit)
{
	(void) fprintf(stderr, "Beginning automatic optimal Argon2 benchmark ...\n");
	(void) fprintf(stderr, "NOTE: This does not test multithreading. Use '-a -p' for thread testing.\n");
	(void) fprintf(stderr, "\n");

	(void) argon2_print_colheaders();

	const argon2_type type = Argon2_id;
	size_t memcost = memlimit;
	size_t timecost = ARGON2_TIMECOST_MIN;
	const size_t threads = 1U;

	bool timecost_raised = false;
	long double elapsed_prev = 0L;
	long double elapsed = 0L;

	// First try at our memory limit and the minimum time cost
	if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
		// This function logs error messages on failure
		return false;

	// If that's still too slow, halve the memory usage until it isn't
	while (elapsed > clocklimit)
	{
		if (memcost <= ARGON2_MEMCOST_MIN)
		{
			(void) fprintf(stderr, "Reached minimum memory and time cost.\n");
			(void) fprintf(stderr, "Algorithm is still too slow; giving up.\n");
			(void) fprintf(stderr, "\n");
			(void) fflush(stderr);
			return true;
		}

		memcost--;

		if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	// Now that it's fast enough, raise the time cost until it isn't
	while (elapsed <= clocklimit)
	{
		timecost++;
		timecost_raised = true;
		elapsed_prev = elapsed;

		if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	// If it was raised, go back to the previous loop's outputs
	if (timecost_raised)
	{
		timecost--;
		elapsed = elapsed_prev;
	}

	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "Recommended parameters:\n");
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	(void) fprintf(stdout, "crypto {\n");
	(void) fprintf(stdout, "\t/* Target: %LFs; Benchmarked: %LFs */\n", clocklimit, elapsed);
	(void) fprintf(stdout, "\targon2_type = \"%s\";\n", argon2_type_to_name(type));
	(void) fprintf(stdout, "\targon2_memcost = %zu; /* %u KiB */ \n", memcost, (1U << memcost));
	(void) fprintf(stdout, "\targon2_timecost = %zu;\n", timecost);
	(void) fprintf(stdout, "\targon2_threads = %zu;\n", threads);
	(void) fprintf(stdout, "};\n");
	(void) fflush(stdout);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return true;
}

#endif /* HAVE_LIBARGON2 */

static bool ATHEME_FATTR_WUR
do_optimal_pbkdf2_benchmark(const long double clocklimit)
{
	(void) fprintf(stderr, "Beginning automatic optimal PBKDF2 benchmark ...\n");
	(void) fprintf(stderr, "NOTE: This does not test SHA1. Use '-k -d' for SHA1 testing.\n");
	(void) fprintf(stderr, "NOTE: If you wish to support SASL SCRAM logins, please see\n");
	(void) fprintf(stderr, "      the 'doc/SASL-SCRAM-SHA' file, whose parameter advice\n");
	(void) fprintf(stderr, "      takes precedence over the advice given here.\n");
	(void) fprintf(stderr, "\n");

	(void) pbkdf2_print_colheaders();

	long double elapsed_sha256;
	long double elapsed_sha512;
	long double elapsed;

	enum digest_algorithm md;

	if (! benchmark_pbkdf2(DIGALG_SHA2_512, PBKDF2_ITERCNT_MAX, &elapsed_sha512))
		// This function logs error messages on failure
		return false;

	if (! benchmark_pbkdf2(DIGALG_SHA2_256, PBKDF2_ITERCNT_MAX, &elapsed_sha256))
		// This function logs error messages on failure
		return false;

	if (elapsed_sha256 < elapsed_sha512)
	{
		md = DIGALG_SHA2_256;
		elapsed = elapsed_sha256;
	}
	else
	{
		md = DIGALG_SHA2_512;
		elapsed = elapsed_sha512;
	}

	const char *const mdname = md_digest_to_name(md);
	size_t iterations = (size_t) (PBKDF2_ITERCNT_MAX / (elapsed / clocklimit));
	iterations -= (iterations % 1000U);

	while (elapsed > clocklimit)
	{
		if (iterations <= PBKDF2_ITERCNT_MIN)
		{
			(void) fprintf(stderr, "Reached minimum iteration count.\n");
			(void) fprintf(stderr, "Algorithm is still too slow; giving up.\n");
			(void) fprintf(stderr, "\n");
			(void) fflush(stderr);
			return true;
		}

		iterations -= 1000U;

		if (! benchmark_pbkdf2(md, iterations, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "Recommended parameters:\n");
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	(void) fprintf(stdout, "crypto {\n");
	(void) fprintf(stdout, "\t/* Target: %LFs; Benchmarked: %LFs */\n", clocklimit, elapsed);
	(void) fprintf(stdout, "\tpbkdf2v2_digest = \"%s\";\n", mdname);
	(void) fprintf(stdout, "\tpbkdf2v2_rounds = %zu;\n", iterations);
	(void) fprintf(stdout, "};\n");
	(void) fflush(stdout);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return true;
}

bool ATHEME_FATTR_WUR
do_optimal_benchmarks(const long double clocklimit, const unsigned int memlimit)
{
#ifdef HAVE_LIBARGON2
	if (! do_optimal_argon2_benchmark(clocklimit, memlimit))
		// This function logs error messages on failure
		return false;
#endif
	if (! do_optimal_pbkdf2_benchmark(clocklimit))
		// This function logs error messages on failure
		return false;

	return true;
}
