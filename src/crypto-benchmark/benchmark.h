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

#ifndef ATHEME_SRC_CRYPTO_BENCHMARK_BENCHMARK_H
#define ATHEME_SRC_CRYPTO_BENCHMARK_BENCHMARK_H 1

#include <atheme/attributes.h>      // ATHEME_FATTR_WUR
#include <atheme/digest.h>          // enum digest_algorithm
#include <atheme/stdheaders.h>      // bool
#include <atheme/sysconf.h>         // HAVE_*

#ifdef HAVE_LIBARGON2
#  include <argon2.h>               // argon2_type
#endif

#define BENCH_MAX(a, b)             (((a) <= (b)) ? (b) : (a))
#define BENCH_MIN(a, b)             (((a) <= (b)) ? (a) : (b))

#define BENCH_RUN_OPTIONS_NONE      0x0000U
#define BENCH_RUN_OPTIONS_OPTIMAL   0x0001U
#define BENCH_RUN_OPTIONS_ARGON2    0x0002U
#define BENCH_RUN_OPTIONS_PBKDF2    0x0004U

bool benchmark_init(void) ATHEME_FATTR_WUR;

#ifdef HAVE_LIBARGON2
argon2_type argon2_name_to_type(const char *);
void argon2_print_colheaders(void);
void argon2_print_rowstats(argon2_type, size_t, size_t, size_t, long double);
bool benchmark_argon2(argon2_type, size_t, size_t, size_t, long double *) ATHEME_FATTR_WUR;
#endif

enum digest_algorithm md_name_to_digest(const char *);
const char *md_digest_to_name(enum digest_algorithm);
void pbkdf2_print_colheaders(void);
void pbkdf2_print_rowstats(enum digest_algorithm, size_t, long double);
bool benchmark_pbkdf2(enum digest_algorithm, size_t, long double *) ATHEME_FATTR_WUR;

#endif /* !ATHEME_SRC_CRYPTO_BENCHMARK_BENCHMARK_H */
