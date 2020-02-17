/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Atheme Project (http://atheme.org/)
 */

#include <atheme.h>

#define RAWHASH_DIGEST_ALG      DIGALG_SHA1
#define RAWHASH_DIGEST_LEN      DIGEST_MDLEN_SHA1
#define RAWHASH_MODULE_NAME     "rawsha1"

#include "rawhash.c"
