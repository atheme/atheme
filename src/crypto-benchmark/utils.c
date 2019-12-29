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

#include <atheme/digest.h>      // DIGALG_*
#include <atheme/stdheaders.h>  // (everything else)
#include <atheme/sysconf.h>     // HAVE_LIBARGON2

#include "utils.h"              // self-declarations

#ifdef HAVE_LIBARGON2

const char *
argon2_type_to_name(const argon2_type b_type)
{
	return argon2_type2string(b_type, 0);
}

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
	(void) fprintf(stderr, "%10s %13uK %10zu %10zu %13LFs\n", argon2_type_to_name(type),
	                       (1U << memcost), timecost, threads, elapsed);
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
