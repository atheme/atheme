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

#include <atheme/attributes.h>  // ATHEME_FATTR_WUR
#include <atheme/digest.h>      // enum digest_algorithm
#include <atheme/stdheaders.h>  // bool
#include <atheme/sysconf.h>     // HAVE_LIBARGON2

#ifdef HAVE_LIBARGON2
#  include <argon2.h>           // argon2_type
#endif

bool benchmark_init(void) ATHEME_FATTR_WUR;

#ifdef HAVE_LIBARGON2
bool benchmark_argon2(const argon2_type, size_t, size_t, size_t, long double *) ATHEME_FATTR_WUR;
#endif

bool benchmark_pbkdf2(enum digest_algorithm, size_t, long double *) ATHEME_FATTR_WUR;

#endif /* !ATHEME_SRC_CRYPTO_BENCHMARK_BENCHMARK_H */
