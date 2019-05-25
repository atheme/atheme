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

#include <atheme/digest.h>
#include <atheme/libathemecore.h>
#include <atheme/random.h>

#define BUFSIZE 1024

static const enum digest_algorithm mds[] = { DIGALG_MD5, DIGALG_SHA1, DIGALG_SHA2_256, DIGALG_SHA2_512 };
static const size_t mds_count = (sizeof mds) / (sizeof mds[0]);

static const size_t saltlengths[] = { 32 };
static const size_t saltlengths_count = (sizeof saltlengths) / (sizeof saltlengths[0]);

static const size_t passlengths[] = { 8, 64, 288 };
static const size_t passlengths_count = (sizeof passlengths) / (sizeof passlengths[0]);

static const size_t rounds[] = { 10000, 32000, 64000, 128000, 256000, 512000, 1000000 };
static const size_t rounds_count = (sizeof rounds) / (sizeof rounds[0]);

static const char *
md_name(const enum digest_algorithm md)
{
	switch (md)
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

static bool
run(const enum digest_algorithm md, const size_t saltlength, const size_t passlength, const size_t itercnt)
{
	unsigned char buf[BUFSIZE];
	unsigned char salt[BUFSIZE];
	char password[BUFSIZE];
	char begin_str[BUFSIZE];
	char end_str[BUFSIZE];
	struct timespec begin;
	struct timespec end;

	const size_t mdlen = digest_size_alg(md);

	(void) atheme_random_buf(salt, saltlength);
	(void) atheme_random_str(password, passlength);
	(void) memset(&begin, 0x00, sizeof begin);
	(void) memset(&end, 0x00, sizeof end);

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin) != 0)
	{
		(void) perror("clock_gettime");
		return false;
	}
	if (! digest_oneshot_pbkdf2(md, password, passlength, salt, saltlength, itercnt, buf, mdlen))
	{
		(void) perror("digest_oneshot_pbkdf2");
		return false;
	}
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) != 0)
	{
		(void) perror("clock_gettime");
		return false;
	}

	const long double nsec_per_sec = (long double) 1000000000.0;
	const long double begin_ld = ((long double) begin.tv_sec) + (((long double) begin.tv_nsec) / nsec_per_sec);
	const long double end_ld = ((long double) end.tv_sec) + (((long double) end.tv_nsec) / nsec_per_sec);
	const long double elapsed = (end_ld - begin_ld);

	(void) snprintf(begin_str, sizeof begin_str, "%lu.%.9lu", (unsigned long) begin.tv_sec, (unsigned long) begin.tv_nsec);
	(void) snprintf(end_str, sizeof end_str, "%lu.%.9lu", (unsigned long) end.tv_sec, (unsigned long) end.tv_nsec);
	(void) fprintf(stderr, "%10s %10zu %10zu %10zu %16s %16s %12LF\n", md_name(md),
	               saltlength, passlength, itercnt, begin_str, end_str, elapsed);

	return true;
}

int
main(void)
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	(void) fprintf(stderr, "\nUsing digest frontend: %s\n\n", digest_get_frontend_info());

	(void) fprintf(stderr, "%-10s %-10s %-10s %-10s %-16s %-16s %-12s\n",
	                       "Digest", "SaltLen", "PassLen", "Rounds", "Begin", "End", "Elapsed");

	(void) fprintf(stderr, "---------- ---------- ---------- ---------- ---------------- ---------------- ------------\n");

	for (size_t i = 0; i < mds_count; i++)
		for (size_t j = 0; j < saltlengths_count; j++)
			for (size_t k = 0; k < passlengths_count; k++)
				for (size_t l = 0; l < rounds_count; l++)
					if (! run(mds[i], saltlengths[j], passlengths[k], rounds[l]))
						return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
