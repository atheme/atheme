/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme/attributes.h>      // ATHEME_FATTR_WUR
#include <atheme/bcrypt.h>          // ATHEME_BCRYPT_*
#include <atheme/argon2.h>          // ATHEME_ARGON2_*
#include <atheme/digest.h>          // DIGALG_*
#include <atheme/i18n.h>            // _() (gettext)
#include <atheme/pbkdf2.h>          // PBKDF2_*
#include <atheme/scrypt.h>          // ATHEME_SCRYPT_*
#include <atheme/stdheaders.h>      // (everything else)
#include <atheme/sysconf.h>         // HAVE_*, ATHEME_API_*

#ifdef HAVE_LIBARGON2
#  include <argon2.h>               // argon2_type, argon2_type2string()
#endif

#include "benchmark.h"              // (everything else)
#include "optimal.h"                // self-declarations

#ifdef HAVE_LIBARGON2

static bool ATHEME_FATTR_WUR
do_optimal_argon2_benchmark(const long double optimal_clocklimit, const size_t optimal_memlimit)
{
	(void) bench_print("");
	(void) bench_print("");
	(void) bench_print(_("Beginning automatic optimal Argon2 benchmark ..."));

	(void) bench_print("");
	(void) bench_print(_(""
		"NOTICE: This does not test multithreading.\n"
		"Use '-a -p' for thread testing."
	));

	(void) argon2_print_colheaders();

	long double elapsed_prev = 0L;
	long double elapsed = 0L;
	size_t timecost_prev = 0U;

	const argon2_type type = Argon2_id;
	size_t memcost = optimal_memlimit;
	size_t timecost = ATHEME_ARGON2_TIMECOST_MIN;
	const size_t threads = 1ULL;

	// First try at our memory limit and the minimum time cost
	if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
		// This function logs error messages on failure
		return false;

	// If that's still too slow, halve the memory usage until it isn't
	while (elapsed > optimal_clocklimit)
	{
		if (memcost <= ATHEME_ARGON2_MEMCOST_MIN)
		{
			(void) bench_print("");
			(void) bench_print(_("Reached minimum memory and time cost!"));
			(void) bench_print(_("Algorithm is too slow; giving up."));
			return true;
		}

		memcost--;

		if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	// Now that it's fast enough, raise the time cost until it isn't
	while (elapsed < optimal_clocklimit)
	{
		elapsed_prev = elapsed;
		timecost_prev = timecost;
		timecost++;

		if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	// If it was raised, go back to the previous loop's outputs (now that it's too slow)
	if (timecost_prev)
	{
		elapsed = elapsed_prev;
		timecost = timecost_prev;
	}

	(void) bench_print("");
	(void) bench_print(_("Recommended parameters:"));
	(void) bench_print("");

	(void) fprintf(stdout, "crypto {\n");
	(void) fprintf(stdout, _("\t/* Target: %LFs; Benchmarked: %LFs */\n"), optimal_clocklimit, elapsed);
	(void) fprintf(stdout, "\targon2_type = \"%s\";\n", argon2_type2string(type, 0));
	(void) fprintf(stdout, "\targon2_memcost = %zu; /* %s */ \n", memcost, memory_power2k_to_str(memcost));
	(void) fprintf(stdout, "\targon2_timecost = %zu;\n", timecost);
	(void) fprintf(stdout, "\targon2_threads = %zu;\n", threads);
	(void) fprintf(stdout, "};\n");
	(void) fflush(stdout);

	return true;
}

#endif /* HAVE_LIBARGON2 */

#ifdef HAVE_LIBSODIUM_SCRYPT

static bool ATHEME_FATTR_WUR
do_optimal_scrypt_benchmark(const long double optimal_clocklimit, const size_t optimal_memlimit)
{
	(void) bench_print("");
	(void) bench_print("");
	(void) bench_print(_("Beginning automatic optimal scrypt benchmark ..."));

	(void) scrypt_print_colheaders();

	long double elapsed_prev = 0L;
	long double elapsed = 0L;
	size_t opslimit_prev = 0U;

	/* For tuning, the recommendation is set opslimit to (memlimit / 32),
	 * but we interpret memlimit as a KiB power of 2, so we need to make
	 * memlimit a power of two, and then multiply it by 1024 (to make it
	 * a value in KiB), and then divide it by 32. Those last 2 operations
	 * are equivalent to just multiplying by 32. The benchmark_scrypt()
	 * function itself takes care of the above calculations as far as the
	 * memory limit is concerned.
	 */
	size_t memlimit = optimal_memlimit;
	size_t opslimit = ((1ULL << memlimit) * 32ULL);

	// First try at our memory limit and the corresponding default opslimit
	if (! benchmark_scrypt(memlimit, opslimit, &elapsed))
		// This function logs error messages on failure
		return false;

	// If that's still too slow, halve the memory limit and re-calculate opslimit until it isn't
	while (elapsed > optimal_clocklimit)
	{
		if (memlimit <= ATHEME_SCRYPT_MEMLIMIT_MIN)
		{
			(void) bench_print("");
			(void) bench_print(_("Reached minimum memory limit!"));
			(void) bench_print(_("Algorithm is too slow; giving up."));
			return true;
		}

		memlimit--;
		opslimit = ((1ULL << memlimit) * 32ULL);

		if (! benchmark_scrypt(memlimit, opslimit, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	// Now that it's fast enough, raise the opslimit until it isn't
	while (elapsed < optimal_clocklimit)
	{
		elapsed_prev = elapsed;
		opslimit_prev = opslimit;
		opslimit *= 2U;

		if (! benchmark_scrypt(memlimit, opslimit, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	// If it was raised, go back to the previous loop's outputs (now that it's too slow)
	if (opslimit_prev)
	{
		elapsed = elapsed_prev;
		opslimit = opslimit_prev;
	}

	(void) bench_print("");
	(void) bench_print(_("Recommended parameters:"));
	(void) bench_print("");

	(void) fprintf(stdout, "crypto {\n");
	(void) fprintf(stdout, _("\t/* Target: %LFs; Benchmarked: %LFs */\n"), optimal_clocklimit, elapsed);
	(void) fprintf(stdout, "\tscrypt_memlimit = %zu; /* %s */ \n", memlimit, memory_power2k_to_str(memlimit));
	(void) fprintf(stdout, "\tscrypt_opslimit = %zu;\n", opslimit);
	(void) fprintf(stdout, "};\n");
	(void) fflush(stdout);

	return true;
}

#endif /* HAVE_LIBSODIUM_SCRYPT */

static bool ATHEME_FATTR_WUR
do_optimal_bcrypt_benchmark(const long double optimal_clocklimit)
{
	(void) bench_print("");
	(void) bench_print("");
	(void) bench_print(_("Beginning automatic optimal bcrypt benchmark ..."));

	(void) bcrypt_print_colheaders();

	long double elapsed_prev = 0L;
	long double elapsed = 0L;

	unsigned int rounds_prev = 0U;
	unsigned int rounds = ATHEME_BCRYPT_ROUNDS_MIN;

	// First try at the minimum rounds
	if (! benchmark_bcrypt(rounds, &elapsed))
		// This function logs error messages on failure
		return false;

	if (elapsed > optimal_clocklimit)
	{
		(void) bench_print("");
		(void) bench_print(_("Reached minimum rounds limit!"));
		(void) bench_print(_("Algorithm is too slow; giving up."));
		return true;
	}

	// If that's still too fast, raise the rounds until it isn't
	while (elapsed < optimal_clocklimit)
	{
		elapsed_prev = elapsed;
		rounds_prev = rounds;
		rounds++;

		if (! benchmark_bcrypt(rounds, &elapsed))
			// This function logs error messages on failure
			return false;

		if (rounds == ATHEME_BCRYPT_ROUNDS_MAX)
		{
			rounds_prev = 0U;
			break;
		}
	}

	// If it was raised, go back to the previous loop's outputs (now that it's too slow)
	if (rounds_prev)
	{
		elapsed = elapsed_prev;
		rounds = rounds_prev;
	}

	(void) bench_print("");
	(void) bench_print(_("Recommended parameters:"));
	(void) bench_print("");

	(void) fprintf(stdout, "crypto {\n");
	(void) fprintf(stdout, _("\t/* Target: %LFs; Benchmarked: %LFs */\n"), optimal_clocklimit, elapsed);
	(void) fprintf(stdout, "\tbcrypt_cost = %u;\n", rounds);
	(void) fprintf(stdout, "};\n");
	(void) fflush(stdout);

	return true;
}

static bool ATHEME_FATTR_WUR
do_optimal_pbkdf2_benchmark(const long double optimal_clocklimit, const bool with_sasl_scram)
{
	(void) bench_print("");
	(void) bench_print("");
	(void) bench_print(_("Beginning automatic optimal PBKDF2 benchmark ..."));

#if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_INTERNAL)
	(void) bench_print("");
	(void) bench_print(_(""
		"NOTICE: This program may perform significantly better if you build it\n"
		"        against a supported third-party cryptographic digest library!"
	));
#endif

	if (! with_sasl_scram)
	{
		(void) bench_print("");
		(void) bench_print(_(""
			"WARNING: If you wish to support SASL SCRAM (RFC 5802) logins, please see\n"
			"         the 'doc/SASL-SCRAM' file in the source code repository, whose\n"
			"         parameter advice takes precedence over the advice given by this\n"
			"         benchmark utility! Pass -i to this utility to enable SASL SCRAM\n"
			"         support and silence this warning."
		));
	}

	const size_t initial = ((with_sasl_scram) ? CYRUS_SASL_ITERCNT_MAX : PBKDF2_ITERCNT_MAX);

	(void) bench_print("");
	(void) bench_print(_("Selecting iterations starting point: %zu"), initial);

	(void) pbkdf2_print_colheaders();

	long double elapsed_sha512 = 0L;
	long double elapsed_sha256 = 0L;
	long double elapsed;

	enum digest_algorithm md;

	if (! benchmark_pbkdf2(DIGALG_SHA1, initial, with_sasl_scram, NULL))
		// This function logs error messages on failure
		return false;

	if (! benchmark_pbkdf2(DIGALG_SHA2_256, initial, with_sasl_scram, &elapsed_sha256))
		// This function logs error messages on failure
		return false;

	if (! benchmark_pbkdf2(DIGALG_SHA2_512, initial, with_sasl_scram, &elapsed_sha512))
		// This function logs error messages on failure
		return false;

	/* This if / else if / else intentionally does not consider SHA1, which could hardly
	 * be considered "optimal"! Always recommend SCRAM-SHA-256 over SCRAM-SHA-512 due to
	 * poor client support for the latter.
	 */
	if (with_sasl_scram || elapsed_sha256 <= elapsed_sha512)
	{
		md = DIGALG_SHA2_256;
		elapsed = elapsed_sha256;
	}
	else
	{
		// This *can* happen!
		md = DIGALG_SHA2_512;
		elapsed = elapsed_sha512;
	}

	/* PBKDF2 is pretty linear: There's only one parameter (iteration count), and it has
	 * almost perfect scaling on the algorithm's runtime. This enables a very simplified
	 * optimal parameter discovery process, compared to the other functions above.
	 */
	const char *const mdname = md_digest_to_name(md, with_sasl_scram);
	size_t iterations = (size_t) (1.1L * (initial * (optimal_clocklimit / elapsed)));
	iterations += (1000U - (iterations % 1000U));
	iterations = BENCH_MIN(PBKDF2_ITERCNT_MAX, iterations);
	iterations = BENCH_MAX(PBKDF2_ITERCNT_MIN, iterations);

	if (with_sasl_scram)
		// The adjustments above might have raised it above the recommended maximum
		iterations = BENCH_MIN(CYRUS_SASL_ITERCNT_MAX, iterations);

	(void) bench_print("");
	(void) bench_print(_("Selecting optimal algorithm: %s"), mdname);

	if (iterations != initial || elapsed > optimal_clocklimit)
		(void) pbkdf2_print_colheaders();

	if (iterations != initial && ! benchmark_pbkdf2(md, iterations, with_sasl_scram, &elapsed))
		// This function logs error messages on failure
		return false;

	while (elapsed > optimal_clocklimit)
	{
		if (iterations <= PBKDF2_ITERCNT_MIN)
		{
			(void) bench_print("");
			(void) bench_print(_("Reached minimum iteration count!"));
			(void) bench_print(_("Algorithm is too slow; giving up."));
			return true;
		}

		// Shave digits after thousands off while reducing by a thousand too
		iterations -= (1000U + (iterations % 1000U));

		if (! benchmark_pbkdf2(md, iterations, with_sasl_scram, &elapsed))
			// This function logs error messages on failure
			return false;
	}

	(void) bench_print("");
	(void) bench_print(_("Recommended parameters:"));
	(void) bench_print("");

	(void) fprintf(stdout, "crypto {\n");
	(void) fprintf(stdout, _("\t/* Target: %LFs; Benchmarked: %LFs */\n"), optimal_clocklimit, elapsed);
	(void) fprintf(stdout, "\tpbkdf2v2_digest = \"%s\";\n", mdname);
	(void) fprintf(stdout, "\tpbkdf2v2_rounds = %zu;\n", iterations);
	(void) fprintf(stdout, "};\n");
	(void) fflush(stdout);

	return true;
}

bool ATHEME_FATTR_WUR
do_optimal_benchmarks(const long double optimal_clocklimit, const size_t ATHEME_VATTR_MAYBE_UNUSED optimal_memlimit,
                      const bool ATHEME_VATTR_MAYBE_UNUSED optimal_memlimit_given, const bool with_sasl_scram)
{
#ifdef HAVE_ANY_MEMORY_HARD_ALGORITHM
	if (! optimal_memlimit_given)
	{
		(void) bench_print("");
		(void) bench_print("");
		(void) bench_print(_(""
			"NOTICE: Please be sure to specify -l/--optimal-memory-limit\n"
			"        appropriately for this machine!"
		));
	}
#endif

#ifdef HAVE_LIBARGON2
	if (! do_optimal_argon2_benchmark(optimal_clocklimit, optimal_memlimit))
		// This function logs error messages on failure
		return false;
#endif

#ifdef HAVE_LIBSODIUM_SCRYPT
	if (! do_optimal_scrypt_benchmark(optimal_clocklimit, optimal_memlimit))
		// This function logs error messages on failure
		return false;
#endif

	if (! do_optimal_bcrypt_benchmark(optimal_clocklimit))
		// This function logs error messages on failure
		return false;

	if (! do_optimal_pbkdf2_benchmark(optimal_clocklimit, with_sasl_scram))
		// This function logs error messages on failure
		return false;

	(void) fsync(fileno(stdout));
	return true;
}
