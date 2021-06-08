/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#ifndef ATHEME_SRC_CRYPTO_BENCHMARK_SELFTESTS_H
#define ATHEME_SRC_CRYPTO_BENCHMARK_SELFTESTS_H 1

#include <atheme/attributes.h>      // ATHEME_FATTR_WUR
#include <atheme/stdheaders.h>      // bool

bool do_crypto_selftests(void) ATHEME_FATTR_WUR;

#endif /* !ATHEME_SRC_CRYPTO_BENCHMARK_SELFTESTS_H */
