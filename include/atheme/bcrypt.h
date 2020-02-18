/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
 */

#ifndef ATHEME_INC_BCRYPT_H
#define ATHEME_INC_BCRYPT_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

#define ATHEME_BCRYPT_SALTLEN       16U
#define ATHEME_BCRYPT_HASHLEN       24U

#define ATHEME_BCRYPT_ROUNDS_MIN    4U
#define ATHEME_BCRYPT_ROUNDS_DEF    7U
#define ATHEME_BCRYPT_ROUNDS_MAX    31U

#define ATHEME_BCRYPT_VERSION_MINOR 'b'

bool atheme_eks_bf_compute(const char *, unsigned int, unsigned int,
                           const unsigned char salt[static ATHEME_BCRYPT_SALTLEN],
                           unsigned char hash[static ATHEME_BCRYPT_HASHLEN]) ATHEME_FATTR_WUR;

bool atheme_eks_bf_testsuite_run(void) ATHEME_FATTR_WUR;

#endif /* !ATHEME_INC_BCRYPT_H */
