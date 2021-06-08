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
#define BENCH_RUN_OPTIONS_TESTONLY  0x0001U
#define BENCH_RUN_OPTIONS_OPTIMAL   0x0002U
#define BENCH_RUN_OPTIONS_ARGON2    0x0004U
#define BENCH_RUN_OPTIONS_SCRYPT    0x0008U
#define BENCH_RUN_OPTIONS_BCRYPT    0x0010U
#define BENCH_RUN_OPTIONS_PBKDF2    0x0020U

#if defined(HAVE_LIBARGON2) || defined(HAVE_LIBSODIUM_SCRYPT)
#  define HAVE_ANY_MEMORY_HARD_ALGORITHM 1
#endif

void bench_print(const char *, ...) ATHEME_FATTR_PRINTF(1, 2);
bool benchmark_init(void) ATHEME_FATTR_WUR;

#ifdef HAVE_ANY_MEMORY_HARD_ALGORITHM
const char *memory_power2k_to_str(size_t);
#endif

#ifdef HAVE_LIBARGON2
argon2_type argon2_name_to_type(const char *);
void argon2_print_colheaders(void);
void argon2_print_rowstats(argon2_type, size_t, size_t, size_t, long double);
bool benchmark_argon2(argon2_type, size_t, size_t, size_t, long double *) ATHEME_FATTR_WUR;
#endif

#ifdef HAVE_LIBSODIUM_SCRYPT
void scrypt_print_colheaders(void);
void scrypt_print_rowstats(size_t, size_t, long double);
bool benchmark_scrypt(size_t, size_t, long double *) ATHEME_FATTR_WUR;
#endif

void bcrypt_print_colheaders(void);
void bcrypt_print_rowstats(unsigned int, long double);
bool benchmark_bcrypt(unsigned int, long double *) ATHEME_FATTR_WUR;

enum digest_algorithm md_name_to_digest(const char *);
const char *md_digest_to_name(enum digest_algorithm, bool);
void pbkdf2_print_colheaders(void);
void pbkdf2_print_rowstats(enum digest_algorithm, size_t, bool, long double);
bool benchmark_pbkdf2(enum digest_algorithm, size_t, bool, long double *) ATHEME_FATTR_WUR;

#endif /* !ATHEME_SRC_CRYPTO_BENCHMARK_BENCHMARK_H */
