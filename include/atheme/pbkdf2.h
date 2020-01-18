/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Data structures and macros for the PBKDF2 crypto modules and the SASL SCRAM
 * module.
 */

#ifndef ATHEME_INC_PBKDF2_H
#define ATHEME_INC_PBKDF2_H 1

#include <atheme/base64.h>
#include <atheme/digest.h>
#include <atheme/stdheaders.h>

#define PBKDF2_LEGACY_ITERCNT           128000U
#define PBKDF2_LEGACY_SALTLEN           16U
#define PBKDF2_LEGACY_PARAMLEN          (PBKDF2_LEGACY_SALTLEN + (2 * DIGEST_MDLEN_SHA2_512))
#define PBKDF2_LEGACY_MODULE_NAME       "crypto/pbkdf2"

#define PBKDF2V2_CRYPTO_MODULE_NAME     "crypto/pbkdf2v2"

#define PBKDF2_FN_PREFIX                "$z$%u$%u$"
#define PBKDF2_FN_LOADSALT              PBKDF2_FN_PREFIX "%[" BASE64_ALPHABET_RFC4648 "]$"
#define PBKDF2_FN_LOADHASH              PBKDF2_FN_LOADSALT "%[" BASE64_ALPHABET_RFC4648 "]"
#define PBKDF2_FS_LOADHASH              PBKDF2_FN_LOADHASH "$%[" BASE64_ALPHABET_RFC4648 "]"

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

#define PBKDF2_PRF_SCRAM_MD5            43U
#define PBKDF2_PRF_SCRAM_SHA1           44U
#define PBKDF2_PRF_SCRAM_SHA2_256       45U
#define PBKDF2_PRF_SCRAM_SHA2_512       46U

#define PBKDF2_PRF_SCRAM_MD5_S64        63U
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

/* Maximum iteration count Cyrus SASL clients will process
 * Taken from <https://github.com/cyrusimap/cyrus-sasl/blob/f76eb971d456619d0f26/plugins/scram.c#L79>
 */
#define CYRUS_SASL_ITERCNT_MAX          0x10000U

struct pbkdf2v2_dbentry
{
	unsigned char           cdg[DIGEST_MDLEN_MAX];                        // PBKDF2 Digest (Computed)
	unsigned char           sdg[DIGEST_MDLEN_MAX];                        // PBKDF2 Digest (Stored)
	unsigned char           ssk[DIGEST_MDLEN_MAX];                        // SCRAM ServerKey (Stored)
	unsigned char           shk[DIGEST_MDLEN_MAX];                        // SCRAM StoredKey (Stored)
	unsigned char           salt[PBKDF2_SALTLEN_MAX];                     // PBKDF2 Salt
	char                    salt64[BASE64_SIZE_STR(PBKDF2_SALTLEN_MAX)];  // PBKDF2 Salt (Base64-encoded)
	size_t                  dl;                                           // Hash/HMAC/PBKDF2 Digest Length
	size_t                  sl;                                           // PBKDF2 Salt Length
	unsigned int            a;                                            // PBKDF2v2 PRF ID (one of the above)
	unsigned int            c;                                            // PBKDF2 Iteration Count
	enum digest_algorithm   md;                                           // Atheme Digest Interface Algorithm ID
	bool                    scram;                                        // Whether to use SCRAM-type credentials
};

struct pbkdf2v2_scram_config
{
	const unsigned int      a;      // PBKDF2v2 PRF ID (one of the above)
	const unsigned int      c;      // PBKDF2 Iteration Count
	const unsigned int      sl;     // PBKDF2 Salt Length
};

typedef void (*pbkdf2v2_scram_confhook_fn)(const struct pbkdf2v2_scram_config *restrict);

struct pbkdf2v2_scram_functions
{
	bool  (*dbextract)(const char *restrict, struct pbkdf2v2_dbentry *restrict);
	bool  (*normalize)(char *restrict, size_t);
	void  (*confhook)(pbkdf2v2_scram_confhook_fn);
};

#endif /* !ATHEME_INC_PBKDF2_H */
