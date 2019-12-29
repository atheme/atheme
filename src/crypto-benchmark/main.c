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

#include <atheme/argon2.h>      // ARGON2_*
#include <atheme/digest.h>      // DIGALG_*
#include <atheme/memory.h>      // sreallocarray()
#include <atheme/pbkdf2.h>      // PBKDF2_*
#include <atheme/stdheaders.h>  // (everything else)
#include <atheme/tools.h>       // string_to_uint()

#include <ext/getopt_long.h>    // mowgli_getopt_option_t, mowgli_getopt_long()

#include "benchmark.h"          // benchmark_*()
#include "optimal.h"            // do_optimal_benchmarks()
#include "utils.h"              // (everything else)

#define BENCH_ARRAY_SIZE(x)     ((sizeof((x))) / (sizeof((x)[0])))
#define BENCH_CLOCKTIME_DEFAULT 0.25L
#define BENCH_MEMLIMIT_DEFAULT  ARGON2_MEMCOST_DEF;

static size_t b_itercounts_default[] = { PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_DEF, PBKDF2_ITERCNT_MAX };
static size_t *b_itercounts = NULL;
static size_t b_itercounts_count = 0;

static enum digest_algorithm b_digests_default[] = { DIGALG_SHA1, DIGALG_SHA2_256, DIGALG_SHA2_512 };
static enum digest_algorithm *b_digests = NULL;
static size_t b_digests_count = 0;

#ifdef HAVE_LIBARGON2

static argon2_type b_types_default[] = { Argon2_id };
static argon2_type *b_types = NULL;
static size_t b_types_count = 0;

static size_t b_memcosts_default[] = { ARGON2_MEMCOST_MIN, ARGON2_MEMCOST_DEF };
static size_t *b_memcosts = NULL;
static size_t b_memcosts_count = 0;

static size_t b_timecosts_default[] = { ARGON2_TIMECOST_MIN, ARGON2_TIMECOST_DEF };
static size_t *b_timecosts = NULL;
static size_t b_timecosts_count = 0;

static size_t b_threads_default[] = { ARGON2_THREADS_DEF };
static size_t *b_threads = NULL;
static size_t b_threads_count = 0;

#endif /* HAVE_LIBARGON2 */

static long double optimal_clocklimit = BENCH_CLOCKTIME_DEFAULT;
static unsigned int optimal_memlimit = BENCH_MEMLIMIT_DEFAULT;

static bool run_optimal_benchmarks = false;
static bool run_argon2_benchmarks = false;
static bool run_pbkdf2_benchmarks = false;

#ifdef HAVE_LIBARGON2
static const char bench_short_opts[] = "hvog:l:an:m:t:p:kc:d:";
#else
static const char bench_short_opts[] = "hvog:kc:d:";
#endif

static const mowgli_getopt_option_t bench_long_opts[] = {
	{       "help",       no_argument, NULL, 'h', 0 },
	{    "version",       no_argument, NULL, 'v', 0 },
	{    "optimal",       no_argument, NULL, 'o', 0 },
	{ "clocklimit", required_argument, NULL, 'g', 0 },
#ifdef HAVE_LIBARGON2
	{   "memlimit", required_argument, NULL, 'l', 0 },
	{     "argon2",       no_argument, NULL, 'a', 0 },
	{       "type", required_argument, NULL, 'n', 0 },
	{     "memory", required_argument, NULL, 'm', 0 },
	{       "time", required_argument, NULL, 't', 0 },
	{    "threads", required_argument, NULL, 'p', 0 },
#endif
	{     "pbkdf2",       no_argument, NULL, 'k', 0 },
	{ "iterations", required_argument, NULL, 'c', 0 },
	{    "digests", required_argument, NULL, 'd', 0 },
	{         NULL,                 0, NULL,  0 , 0 },
};

static inline void
print_usage(void)
{
	(void) fprintf(stderr, "\n"
	"  Usage: " PACKAGE_TARNAME "-crypto-benchmark [-h | -v]\n"
	"  Usage: " PACKAGE_TARNAME "-crypto-benchmark -o [-g <clocklimit>] [-l <memlimit>]\n"
#ifdef HAVE_LIBARGON2
	"         " PACKAGE_TARNAME "-crypto-benchmark -a [-m ...] [-n ...] [-p ...] [-t ...]\n"
#endif
	"         " PACKAGE_TARNAME "-crypto-benchmark -k [-c ...] [-d ...]\n"
	"\n"
	"  -h/--help        Display this help information and exit\n"
	"  -v/--version     Display program version and exit\n"
	"\n"
	"  -o/--optimal     Perform an automatic optimal parameter benchmark\n"
	"  -g/--clocklimit  Wall clock time limit for optimal benchmarking (seconds)\n"
#ifdef HAVE_LIBARGON2
	"  -l/--memlimit    Memory limit for optimal benchmarking (power of 2, in KiB)\n"
	"                     For example, '-l 16' means 2^16 KiB; 65536 KiB; 64 MiB\n"
#else
	"  -l/--memlimit    Unsupported\n"
#endif
	"\n"
	"    If one of the above limits are not given, defaults are used.\n"
#ifdef HAVE_LIBARGON2
	"\n"
	"  -a/--argon2      Benchmark the Argon2 code over a variety of configurations\n"
	"  -m/--memory      Comma-separated Argon2 memory costs to benchmark\n"
	"  -n/--type        Comma-separated Argon2 types to benchmark\n"
	"  -p/--threads     Comma-separated Argon2 thread counts to benchmark\n"
	"  -t/--time        Comma-separated Argon2 time costs to benchmark\n"
#endif
	"\n"
	"  -k/--pbkdf2      Benchmark the PBKDF2 code over a variety of configurations\n"
	"  -c/--iterations  Comma-separated PBKDF2 iteration counts to benchmark\n"
	"  -d/--digests     Comma-separated PBKDF2 digest algorithms to benchmark\n"
	"\n"
	"    If one of the comma-separated options are not given, defaults are used.\n"
	"\n");
}

static inline bool
process_uint_option(const int sw, const char *const restrict val, size_t **const restrict arr,
                    size_t *const restrict arr_len, const unsigned int val_min, const unsigned int val_max)
{
	char *opt;
	char *tok;

	if (! (opt = strdup(val)))
	{
		(void) perror("strdup(3)");
		return false;
	}
	while ((tok = strsep(&opt, ",")) != NULL)
	{
		unsigned int b_value = 0;

		if (! string_to_uint(tok, &b_value) || b_value < val_min || b_value > val_max)
		{
			(void) fprintf(stderr, "'%s' is not a valid value for integer option '%c'\n", tok, sw);
			(void) fprintf(stderr, "range of valid values: %u to %u (inclusive)\n", val_min, val_max);
			return false;
		}
		if (! (*arr = sreallocarray(*arr, (*arr_len) + 1, sizeof **arr)))
		{
			(void) perror("sreallocarray()");
			return false;
		}

		(*arr)[(*arr_len)++] = b_value;
	}

	(void) free(opt);
	return true;
}

static bool
process_options(int argc, char *argv[])
{
	char *opt;
	char *tok;
	int c;

	while ((c = mowgli_getopt_long(argc, argv, bench_short_opts, bench_long_opts, NULL)) != -1)
	{
		switch (c)
		{
			case 'h':
				(void) print_usage();
				exit(EXIT_SUCCESS);

			case 'v':
				(void) fprintf(stderr, "%s\n", PACKAGE_STRING);
				exit(EXIT_SUCCESS);

			case 'o':
				run_optimal_benchmarks = true;
				break;

			case 'g':
			{
				errno = 0;

				char *end = NULL;
				const long double ret = strtold(mowgli_optarg, &end);

				if (! ret || (end && *end))
				{
					(void) fprintf(stderr, "'%s' is not a valid value for decimal option '%c'\n",
					                       mowgli_optarg, c);
					return false;
				}

				optimal_clocklimit = ret;
				break;
			}

#ifdef HAVE_LIBARGON2
			case 'l':
				if (! string_to_uint(mowgli_optarg, &optimal_memlimit))
				{
					(void) fprintf(stderr, "'%s' is not a valid value for integer option '%c'\n",
					                       mowgli_optarg, c);
					(void) fprintf(stderr, "range of valid values: %u to %u (inclusive)\n",
					                       ARGON2_MEMCOST_MIN, ARGON2_MEMCOST_MAX);
					return false;
				}
				if (optimal_memlimit < ARGON2_MEMCOST_MIN || optimal_memlimit > ARGON2_MEMCOST_MAX)
				{
					(void) fprintf(stderr, "'%u' is not a valid value for integer option '%c'\n",
					                       optimal_memlimit, c);
					(void) fprintf(stderr, "range of valid values: %u to %u (inclusive)\n",
					                       ARGON2_MEMCOST_MIN, ARGON2_MEMCOST_MAX);
					return false;
				}
				break;

			case 'a':
				run_argon2_benchmarks = true;
				break;

			case 'n':
			{
				if (! (opt = strdup(mowgli_optarg)))
				{
					(void) perror("strdup(3)");
					return false;
				}
				while ((tok = strsep(&opt, ",")) != NULL)
				{
					const argon2_type b_type = argon2_name_to_type(tok);

					if (! (b_types = sreallocarray(b_types, b_types_count + 1, sizeof b_type)))
					{
						(void) perror("sreallocarray()");
						return false;
					}

					b_types[b_types_count++] = b_type;
				}

				(void) free(opt);
				break;
			}

			case 'm':
				if (! process_uint_option(c, mowgli_optarg, &b_memcosts, &b_memcosts_count,
				                          ARGON2_MEMCOST_MIN, ARGON2_MEMCOST_MAX))
					// This function logs error messages on failure
					return false;

				break;

			case 't':
				if (! process_uint_option(c, mowgli_optarg, &b_timecosts, &b_timecosts_count,
				                          ARGON2_TIMECOST_MIN, ARGON2_TIMECOST_MAX))
					// This function logs error messages on failure
					return false;

				break;

			case 'p':
				if (! process_uint_option(c, mowgli_optarg, &b_threads, &b_threads_count,
				                          ARGON2_THREADS_MIN, ARGON2_THREADS_MAX))
					// This function logs error messages on failure
					return false;

				break;
#endif /* HAVE_LIBARGON2 */

			case 'k':
				run_pbkdf2_benchmarks = true;
				break;

			case 'c':
				if (! process_uint_option(c, mowgli_optarg, &b_itercounts, &b_itercounts_count,
				                          PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_MAX))
					// This function logs error messages on failure
					return false;

				break;

			case 'd':
			{
				if (! (opt = strdup(mowgli_optarg)))
				{
					(void) perror("strdup(3)");
					return false;
				}
				while ((tok = strsep(&opt, ",")) != NULL)
				{
					const enum digest_algorithm b_digest = md_name_to_digest(tok);

					if (! (b_digests = sreallocarray(b_digests, b_digests_count + 1,
					                                 sizeof b_digest)))
					{
						(void) perror("sreallocarray()");
						return false;
					}

					b_digests[b_digests_count++] = b_digest;
				}

				(void) free(opt);
				break;
			}

			default:
				return false;
		}
	}

	if (! (run_optimal_benchmarks || run_argon2_benchmarks || run_pbkdf2_benchmarks))
	{
		(void) print_usage();
		return false;
	}

	if (! b_itercounts)
	{
		b_itercounts = b_itercounts_default;
		b_itercounts_count = BENCH_ARRAY_SIZE(b_itercounts_default);
	}
	if (! b_digests)
	{
		b_digests = b_digests_default;
		b_digests_count = BENCH_ARRAY_SIZE(b_digests_default);
	}
#ifdef HAVE_LIBARGON2
	if (! b_types)
	{
		b_types = b_types_default;
		b_types_count = BENCH_ARRAY_SIZE(b_types_default);
	}
	if (! b_memcosts)
	{
		b_memcosts = b_memcosts_default;
		b_memcosts_count = BENCH_ARRAY_SIZE(b_memcosts_default);
	}
	if (! b_timecosts)
	{
		b_timecosts = b_timecosts_default;
		b_timecosts_count = BENCH_ARRAY_SIZE(b_timecosts_default);
	}
	if (! b_threads)
	{
		b_threads = b_threads_default;
		b_threads_count = BENCH_ARRAY_SIZE(b_threads_default);
	}
#endif /* HAVE_LIBARGON2 */

	return true;
}

#ifdef HAVE_LIBARGON2

static inline bool ATHEME_FATTR_WUR
do_argon2_benchmark(const argon2_type type, const size_t memcost, const size_t timecost, const size_t threads)
{
	long double elapsed;

	if (! benchmark_argon2(type, memcost, timecost, threads, &elapsed))
		// This function logs error messages on failure
		return false;

	(void) fprintf(stderr, "%10s %10zu %8zu %4zu %12LF\n", argon2_type_to_name(type),
	                       memcost, timecost, threads, elapsed);
	return true;
}

static bool ATHEME_FATTR_WUR
do_argon2_benchmarks(void)
{
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "Beginning Argon2 benchmark ...\n");
	(void) fprintf(stderr, "\n");

	(void) argon2_print_colheaders();

	for (size_t b_type = 0; b_type < b_types_count; b_type++)
	  for (size_t b_memcost = 0; b_memcost < b_memcosts_count; b_memcost++)
	    for (size_t b_timecost = 0; b_timecost < b_timecosts_count; b_timecost++)
	      for (size_t b_thread = 0; b_thread < b_threads_count; b_thread++)
	        if (! do_argon2_benchmark(b_types[b_type], b_memcosts[b_memcost],
	                                  b_timecosts[b_timecost], b_threads[b_thread]))
	          // This function logs error messages on failure
	          return false;

	return true;
}

#endif /* HAVE_LIBARGON2 */

static inline bool ATHEME_FATTR_WUR
do_pbkdf2_benchmark(const enum digest_algorithm digest, const size_t iterations)
{
	long double elapsed;

	if (! benchmark_pbkdf2(digest, iterations, &elapsed))
		// This function logs error messages on failure
		return false;

	(void) fprintf(stderr, "%8s %8zu %12LF\n", md_digest_to_name(digest), iterations, elapsed);
	return true;
}

static bool ATHEME_FATTR_WUR
do_pbkdf2_benchmarks(void)
{
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "Beginning PBKDF2 benchmark ...\n");
	(void) fprintf(stderr, "\n");

	(void) pbkdf2_print_colheaders();

	for (size_t b_digest = 0; b_digest < b_digests_count; b_digest++)
	  for (size_t b_itercount = 0; b_itercount < b_itercounts_count; b_itercount++)
	    if (! do_pbkdf2_benchmark(b_digests[b_digest], b_itercounts[b_itercount]))
	      // This function logs error messages on failure
	      return false;

	return true;
}

int
main(int argc, char *argv[])
{
	if (! benchmark_init())
		// This function logs error messages on failure
		return EXIT_FAILURE;

	if (! process_options(argc, argv))
		// This function logs error messages on failure
		return EXIT_FAILURE;

	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "%s\n", PACKAGE_STRING);
	(void) fprintf(stderr, "Using digest frontend: %s\n", digest_get_frontend_info());
	(void) fprintf(stderr, "\n");

	if (run_optimal_benchmarks && ! do_optimal_benchmarks(optimal_clocklimit, optimal_memlimit))
		// This function logs error messages on failure
		return EXIT_FAILURE;

#ifdef HAVE_LIBARGON2
	if (run_argon2_benchmarks && ! do_argon2_benchmarks())
		// This function logs error messages on failure
		return EXIT_FAILURE;
#endif /* HAVE_LIBARGON2 */

	if (run_pbkdf2_benchmarks && ! do_pbkdf2_benchmarks())
		// This function logs error messages on failure
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
