/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

#define RAWHASH_DIGEST_ALG      DIGALG_SHA2_512
#define RAWHASH_DIGEST_LEN      DIGEST_MDLEN_SHA2_512
#define RAWHASH_MODULE_NAME     "rawsha512"

#include "rawhash.c"
