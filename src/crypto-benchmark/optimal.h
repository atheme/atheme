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

#ifndef ATHEME_SRC_CRYPTO_BENCHMARK_OPTIMAL_H
#define ATHEME_SRC_CRYPTO_BENCHMARK_OPTIMAL_H 1

#include <atheme/attributes.h>  // ATHEME_FATTR_WUR
#include <atheme/stdheaders.h>  // bool

bool do_optimal_benchmarks(long double, unsigned int) ATHEME_FATTR_WUR;

#endif /* !ATHEME_SRC_CRYPTO_BENCHMARK_OPTIMAL_H */
