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

#ifndef ATHEME_SRC_CRYPTO_BENCHMARK_UTILS_H
#define ATHEME_SRC_CRYPTO_BENCHMARK_UTILS_H 1

#include <atheme/digest.h>      // enum digest_algorithm
#include <atheme/sysconf.h>     // HAVE_LIBARGON2

#ifdef HAVE_LIBARGON2
#  include <argon2.h>           // argon2_type
#endif

#ifdef HAVE_LIBARGON2
argon2_type argon2_name_to_type(const char *);
const char *argon2_type_to_name(argon2_type);
void argon2_print_colheaders(void);
void argon2_print_rowstats(argon2_type, size_t, size_t, size_t, long double);
#endif

enum digest_algorithm md_name_to_digest(const char *);
const char *md_digest_to_name(enum digest_algorithm);
void pbkdf2_print_colheaders(void);
void pbkdf2_print_rowstats(enum digest_algorithm, size_t, long double);

#endif /* !ATHEME_SRC_CRYPTO_BENCHMARK_UTILS_H */
