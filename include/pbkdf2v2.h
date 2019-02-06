/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Data structures and macros for the PBKDF2v2 crypto module.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_PBKDF2V2_H
#define ATHEME_INC_PBKDF2V2_H 1

#include "digest.h"

#define PBKDF2V2_CRYPTO_MODULE_NAME     "crypto/pbkdf2v2"

#define PBKDF2_FN_PREFIX                "$z$%u$%u$"
#define PBKDF2_FN_BASE64                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="

#define PBKDF2_FN_LOADSALT              PBKDF2_FN_PREFIX "%[" PBKDF2_FN_BASE64 "]$"
#define PBKDF2_FN_LOADHASH              PBKDF2_FN_LOADSALT "%[" PBKDF2_FN_BASE64 "]"
#define PBKDF2_FS_LOADHASH              PBKDF2_FN_LOADHASH "$%[" PBKDF2_FN_BASE64 "]"

#define PBKDF2_FN_SAVESALT              PBKDF2_FN_PREFIX "%s$"
#define PBKDF2_FN_SAVEHASH              PBKDF2_FN_SAVESALT "%s"
#define PBKDF2_FS_SAVEHASH              PBKDF2_FN_SAVEHASH "$%s"

#define PBKDF2_PRF_HMAC_SHA1            4U
#define PBKDF2_PRF_HMAC_SHA2_256        5U
#define PBKDF2_PRF_HMAC_SHA2_512        6U

#define PBKDF2_PRF_HMAC_SHA1_S64        24U
#define PBKDF2_PRF_HMAC_SHA2_256_S64    25U
#define PBKDF2_PRF_HMAC_SHA2_512_S64    26U

#define PBKDF2_PRF_SCRAM_SHA1           44U
#define PBKDF2_PRF_SCRAM_SHA2_256       45U
#define PBKDF2_PRF_SCRAM_SHA2_512       46U     /* Not currently specified */

#define PBKDF2_PRF_SCRAM_SHA1_S64       64U
#define PBKDF2_PRF_SCRAM_SHA2_256_S64   65U
#define PBKDF2_PRF_SCRAM_SHA2_512_S64   66U     /* Not currently specified */

#define PBKDF2_PRF_DEFAULT              PBKDF2_PRF_HMAC_SHA2_512_S64

#define PBKDF2_ITERCNT_MIN              10000U
#define PBKDF2_ITERCNT_MAX              5000000U
#define PBKDF2_ITERCNT_DEF              64000U

#define PBKDF2_SALTLEN_MIN              8U
#define PBKDF2_SALTLEN_MAX              64U
#define PBKDF2_SALTLEN_DEF              32U

struct pbkdf2v2_dbentry
{
	unsigned char   cdg[DIGEST_MDLEN_MAX];          // PBKDF2 Digest (Computed)
	unsigned char   sdg[DIGEST_MDLEN_MAX];          // PBKDF2 Digest (Stored)
	unsigned char   ssk[DIGEST_MDLEN_MAX];          // SCRAM-SHA ServerKey (Stored)
	unsigned char   shk[DIGEST_MDLEN_MAX];          // SCRAM-SHA StoredKey (Stored)
	unsigned char   salt[PBKDF2_SALTLEN_MAX];       // PBKDF2 Salt
	char            salt64[PBKDF2_SALTLEN_MAX * 3]; // PBKDF2 Salt (Base64-encoded)
	size_t          dl;                             // Digest Length
	size_t          sl;                             // Salt Length
	unsigned int    md;                             // Atheme Digest Interface Algorithm Identifier
	unsigned int    a;                              // PBKDF2v2 PRF ID (one of the macros above)
	unsigned int    c;                              // PBKDF2 Iteration Count
	bool            scram;                          // Whether to use HMAC-SHA or SCRAM-SHA
};

static const unsigned char ServerKeyStr[] = {

	// ASCII for "Server Key"
	0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x4B, 0x65, 0x79
};

static const unsigned char ClientKeyStr[] = {

	// ASCII for "Client Key"
	0x43, 0x6C, 0x69, 0x65, 0x6E, 0x74, 0x20, 0x4B, 0x65, 0x79
};

typedef void (*pbkdf2v2_confhook_fn)(unsigned int, unsigned int, unsigned int);

struct pbkdf2v2_scram_functions
{
	bool  (*dbextract)(const char *restrict, struct pbkdf2v2_dbentry *restrict);
	bool  (*normalize)(char *restrict, size_t);
	void  (*confhook)(pbkdf2v2_confhook_fn);
};

#endif /* !ATHEME_INC_PBKDF2V2_H */
