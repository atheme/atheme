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

#define PBKDF2_PRF_HMAC_MD5             3U
#define PBKDF2_PRF_HMAC_SHA1            4U
#define PBKDF2_PRF_HMAC_SHA2_256        5U
#define PBKDF2_PRF_HMAC_SHA2_512        6U

#define PBKDF2_PRF_HMAC_MD5_S64         23U
#define PBKDF2_PRF_HMAC_SHA1_S64        24U
#define PBKDF2_PRF_HMAC_SHA2_256_S64    25U
#define PBKDF2_PRF_HMAC_SHA2_512_S64    26U

#define PBKDF2_PRF_SCRAM_SHA1           44U
#define PBKDF2_PRF_SCRAM_SHA2_256       45U
#define PBKDF2_PRF_SCRAM_SHA2_512       46U

#define PBKDF2_PRF_SCRAM_SHA1_S64       64U
#define PBKDF2_PRF_SCRAM_SHA2_256_S64   65U
#define PBKDF2_PRF_SCRAM_SHA2_512_S64   66U

#define PBKDF2_PRF_DEFAULT              PBKDF2_PRF_HMAC_SHA2_512_S64

#define PBKDF2_ITERCNT_MIN              10000U
#define PBKDF2_ITERCNT_MAX              5000000U
#define PBKDF2_ITERCNT_DEF              64000U

#define PBKDF2_SALTLEN_MIN              8U
#define PBKDF2_SALTLEN_MAX              64U
#define PBKDF2_SALTLEN_DEF              32U

struct pbkdf2v2_dbentry
{
	unsigned char           cdg[DIGEST_MDLEN_MAX];                      // PBKDF2 Digest (Computed)
	unsigned char           sdg[DIGEST_MDLEN_MAX];                      // PBKDF2 Digest (Stored)
	unsigned char           ssk[DIGEST_MDLEN_MAX];                      // SCRAM-SHA ServerKey (Stored)
	unsigned char           shk[DIGEST_MDLEN_MAX];                      // SCRAM-SHA StoredKey (Stored)
	unsigned char           salt[PBKDF2_SALTLEN_MAX];                   // PBKDF2 Salt
	char                    salt64[BASE64_SIZE(PBKDF2_SALTLEN_MAX)];    // PBKDF2 Salt (Base64-encoded)
	size_t                  dl;                                         // Hash/HMAC/PBKDF2 Digest Length
	size_t                  sl;                                         // PBKDF2 Salt Length
	unsigned int            a;                                          // PBKDF2v2 PRF ID (one of the above)
	unsigned int            c;                                          // PBKDF2 Iteration Count
	enum digest_algorithm   md;                                         // Atheme Digest Interface Algorithm ID
	bool                    scram;                                      // Whether to use HMAC-SHA or SCRAM-SHA
};

struct pbkdf2v2_scram_config
{
	const unsigned int *    a;      // PBKDF2v2 PRF ID (one of the above)
	const unsigned int *    c;      // PBKDF2 Iteration Count
	const unsigned int *    sl;     // PBKDF2 Salt Length
};

typedef void (*pbkdf2v2_scram_confhook_fn)(const struct pbkdf2v2_scram_config *restrict);

struct pbkdf2v2_scram_functions
{
	bool  (*dbextract)(const char *restrict, struct pbkdf2v2_dbentry *restrict);
	bool  (*normalize)(char *restrict, size_t);
	void  (*confhook)(pbkdf2v2_scram_confhook_fn);
};

#endif /* !ATHEME_INC_PBKDF2V2_H */
