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

#include <atheme/constants.h>       // BUFSIZE, PASSLEN
#include <atheme/digest.h>          // DIGALG_*, digest_oneshot_pbkdf2()
#include <atheme/libathemecore.h>   // libathemecore_early_init()
#include <atheme/pbkdf2.h>          // PBKDF2_*
#include <atheme/random.h>          // atheme_random_*()
#include <atheme/stdheaders.h>      // (everything else)
#include <atheme/sysconf.h>         // PACKAGE_TARNAME
#include <atheme/tools.h>           // string_to_uint()

#define HELP_STRING ""                                                                              \
    "\n"                                                                                            \
    "Usage: " PACKAGE_TARNAME "-pbkdf2-benchmark [-hv]\n"                                           \
    "       " PACKAGE_TARNAME "-pbkdf2-benchmark [-a algs] [-c iters] [-p lengths] [-s lengths]\n"  \
    "\n"                                                                                            \
    "  -h               Display this help information and exit\n"                                   \
    "  -v               Display program version and exit\n"                                         \
    "  -a algs          Comma-separated algorithm name(s) to benchmark\n"                           \
    "  -c iters         Comma-separated iteration count(s) to benchmark\n"                          \
    "  -p lengths       Comma-separated password length(s) to benchmark\n"                          \
    "  -s lengths       Comma-separated salt length(s) to benchmark\n"                              \
    "\n"                                                                                            \
    "  If one of -a/-c/-p/-s is not given, defaults will be used for them.\n"                       \
    "\n"                                                                                            \
    "  NOTE: Password lengths and salt lengths have almost no effect on\n"                          \
    "        the overall computation time for large iteration counts.\n"

static enum digest_algorithm algs_default[] = { DIGALG_SHA1, DIGALG_SHA2_256, DIGALG_SHA2_512 };
static size_t iters_default[] = { PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_DEF, PBKDF2_ITERCNT_MAX };
static size_t passlens_default[] = { PASSLEN };
static size_t saltlens_default[] = { PBKDF2_SALTLEN_DEF };

static const size_t algs_default_count = (sizeof algs_default) / (sizeof algs_default[0]);
static const size_t iters_default_count = (sizeof iters_default) / (sizeof iters_default[0]);
static const size_t passlens_default_count = (sizeof passlens_default) / (sizeof passlens_default[0]);
static const size_t saltlens_default_count = (sizeof saltlens_default) / (sizeof saltlens_default[0]);

static enum digest_algorithm *algs = NULL;
static size_t *iters = NULL;
static size_t *passlens = NULL;
static size_t *saltlens = NULL;

static size_t algs_count = 0;
static size_t iters_count = 0;
static size_t passlens_count = 0;
static size_t saltlens_count = 0;

static const char *
md_alg_to_name(const enum digest_algorithm alg)
{
	switch (alg)
	{
		case DIGALG_MD5:
			return "MD5";
		case DIGALG_SHA1:
			return "SHA1";
		case DIGALG_SHA2_256:
			return "SHA2-256";
		case DIGALG_SHA2_512:
			return "SHA2-512";
	}

	return NULL;
}

static enum digest_algorithm
md_name_to_alg(const char *const restrict name)
{
	if (strcasecmp(name, "MD5") == 0)
		return DIGALG_MD5;
	else if (strcasecmp(name, "SHA1") == 0)
		return DIGALG_SHA1;
	else if (strcasecmp(name, "SHA2-256") == 0)
		return DIGALG_SHA2_256;
	else if (strcasecmp(name, "SHA2-512") == 0)
		return DIGALG_SHA2_512;

	(void) fprintf(stderr, "%s: '%s' is not a valid algorithm name\n", MOWGLI_FUNC_NAME, name);
	(void) fprintf(stderr, "%s: valid names are: MD5, SHA1, SHA2-256, SHA2-512\n", MOWGLI_FUNC_NAME);

	exit(EXIT_FAILURE);
}

static bool
run_benchmark(const enum digest_algorithm alg, const size_t passlen, const size_t saltlen, const size_t iter)
{
	unsigned char buf[BUFSIZE];
	unsigned char salt[BUFSIZE];
	char password[BUFSIZE];
	struct timespec begin;
	struct timespec end;

	const size_t mdlen = digest_size_alg(alg);

	(void) atheme_random_buf(salt, saltlen);
	(void) atheme_random_str(password, passlen);
	(void) memset(&begin, 0x00, sizeof begin);
	(void) memset(&end, 0x00, sizeof end);

	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &begin) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}
	if (! digest_oneshot_pbkdf2(alg, password, passlen, salt, saltlen, iter, buf, mdlen))
	{
		(void) perror("digest_oneshot_pbkdf2()");
		return false;
	}
	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end) != 0)
	{
		(void) perror("clock_gettime(2)");
		return false;
	}

	const long double nsec_per_sec = (long double) 1000000000.0;
	const long double begin_ld = ((long double) begin.tv_sec) + (((long double) begin.tv_nsec) / nsec_per_sec);
	const long double end_ld = ((long double) end.tv_sec) + (((long double) end.tv_nsec) / nsec_per_sec);
	const long double elapsed = (end_ld - begin_ld);

	(void) fprintf(stderr, "%10s %10zu %10zu %10zu %12LF\n", md_alg_to_name(alg), passlen, saltlen, iter, elapsed);
	return true;
}

static bool
process_options(int argc, char *argv[])
{
	int c;
	char *opt;
	char *tok;

	while ((c = getopt(argc, argv, "hva:c:p:s:")) != -1)
	{
		switch (c)
		{
			case 'h':
				(void) fprintf(stderr, "%s\n", HELP_STRING);
				exit(EXIT_SUCCESS);

			case 'v':
				(void) fprintf(stderr, "%s\n", PACKAGE_STRING);
				exit(EXIT_SUCCESS);

			case 'a':
			{
				if (! (opt = strdup(optarg)))
				{
					(void) perror("strdup(3)");
					return false;
				}
				while ((tok = strsep(&opt, ",")) != NULL)
				{
					const enum digest_algorithm alg = md_name_to_alg(tok);

					if (! (algs = realloc(algs, (sizeof alg) * (algs_count + 1))))
					{
						(void) perror("realloc(3)");
						return false;
					}

					algs[algs_count++] = alg;
				}

				(void) free(opt);
				break;
			}

			case 'c':
			{
				if (! (opt = strdup(optarg)))
				{
					(void) perror("strdup(3)");
					return false;
				}
				while ((tok = strsep(&opt, ",")) != NULL)
				{
					unsigned int iter = 0;

					if (! string_to_uint(tok, &iter) || iter < PBKDF2_ITERCNT_MIN || iter > PBKDF2_ITERCNT_MAX)
					{
						(void) fprintf(stderr, "'%s' is not a valid iteration count\n", tok);
						(void) fprintf(stderr, "range of valid values: %u to %u (inclusive)\n",
						                       PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_MAX);
						return false;
					}
					if (! (iters = realloc(iters, (sizeof iter) * (iters_count + 1))))
					{
						(void) perror("realloc(3)");
						return false;
					}

					iters[iters_count++] = iter;
				}

				(void) free(opt);
				break;
			}

			case 'p':
			{
				if (! (opt = strdup(optarg)))
				{
					(void) perror("strdup(3)");
					return false;
				}
				while ((tok = strsep(&opt, ",")) != NULL)
				{
					unsigned int passlen = 0;

					if (! string_to_uint(tok, &passlen) || ! passlen || passlen > PASSLEN)
					{
						(void) fprintf(stderr, "'%s' is not a valid password length\n", tok);
						(void) fprintf(stderr, "range of valid values: %u to %u (inclusive)\n",
						                       1U, PASSLEN);
						return false;
					}
					if (! (passlens = realloc(passlens, (sizeof passlen) * (passlens_count + 1))))
					{
						(void) perror("realloc(3)");
						return false;
					}

					passlens[passlens_count++] = passlen;
				}

				(void) free(opt);
				break;
			}

			case 's':
			{
				if (! (opt = strdup(optarg)))
				{
					(void) perror("strdup(3)");
					return false;
				}
				while ((tok = strsep(&opt, ",")) != NULL)
				{
					unsigned int saltlen = 0;

					if (! string_to_uint(tok, &saltlen) || saltlen < PBKDF2_SALTLEN_MIN || saltlen > PBKDF2_SALTLEN_MAX)
					{
						(void) fprintf(stderr, "'%s' is not a valid salt length\n", tok);
						(void) fprintf(stderr, "range of valid values: %u to %u (inclusive)\n",
						                       PBKDF2_SALTLEN_MIN, PBKDF2_SALTLEN_MAX);
						return false;
					}
					if (! (saltlens = realloc(saltlens, (sizeof saltlen) * (saltlens_count + 1))))
					{
						(void) perror("realloc(3)");
						return false;
					}

					saltlens[saltlens_count++] = saltlen;
				}

				(void) free(opt);
				break;
			}

			default:
				return false;
		}
	}

	return true;
}

int
main(int argc, char *argv[])
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	if (! process_options(argc, argv))
		return EXIT_FAILURE;

	(void) fprintf(stderr, "\nUsing digest frontend: %s\n\n", digest_get_frontend_info());

	(void) fprintf(stderr, "%-10s %-10s %-10s %-10s %-12s\n",
	                       "Digest", "PassLen", "SaltLen", "Rounds", "Elapsed");

	(void) fprintf(stderr, "---------- ---------- ---------- ---------- ------------\n");

	if (! algs)
	{
		algs = algs_default;
		algs_count = algs_default_count;
	}
	if (! passlens)
	{
		passlens = passlens_default;
		passlens_count = passlens_default_count;
	}
	if (! saltlens)
	{
		saltlens = saltlens_default;
		saltlens_count = saltlens_default_count;
	}
	if (! iters)
	{
		iters = iters_default;
		iters_count = iters_default_count;
	}

	for (size_t alg = 0; alg < algs_count; alg++)
		for (size_t passlen = 0; passlen < passlens_count; passlen++)
			for (size_t saltlen = 0; saltlen < saltlens_count; saltlen++)
				for (size_t iter = 0; iter < iters_count; iter++)
					if (! run_benchmark(algs[alg], passlens[passlen], saltlens[saltlen], iters[iter]))
						return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
