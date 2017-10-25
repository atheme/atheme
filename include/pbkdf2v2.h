/*
 * Copyright (c) 2017 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures and macros for the PBKDF2v2 crypto module.
 */

#ifndef PBKDF2V2_H
#define PBKDF2V2_H

#include <openssl/evp.h>

#define PBKDF2_PRF_HMAC_SHA1        4U
#define PBKDF2_PRF_HMAC_SHA2_256    5U
#define PBKDF2_PRF_HMAC_SHA2_512    6U

#define PBKDF2_PRF_SCRAM_SHA1       44U
#define PBKDF2_PRF_SCRAM_SHA2_256   45U
#define PBKDF2_PRF_SCRAM_SHA2_512   46U     /* Not currently specified */

struct pbkdf2v2_parameters
{
	const EVP_MD    *md;                    // OpenSSL EVP(3) Digest Method
	unsigned char    cdg[EVP_MAX_MD_SIZE+1];  // PBKDF2 Digest (Computed)
	unsigned char    sdg[EVP_MAX_MD_SIZE+1];  // PBKDF2 Digest (Stored)
	unsigned char    ssk[EVP_MAX_MD_SIZE+1];  // SCRAM-SHA ServerKey (Stored)
	unsigned char    shk[EVP_MAX_MD_SIZE+1];  // SCRAM-SHA StoredKey (Stored)
	char             salt[0x2000];          // PBKDF2 Salt
	size_t           dl;                    // Digest Length
	size_t           sl;                    // Salt Length
	unsigned int     a;                     // PRF ID (one of the macros above)
	unsigned int     c;                     // PBKDF2 Iteration Count
	bool             scram;                 // Whether to use HMAC-SHA or SCRAM-SHA
};

static const unsigned char ServerKeyStr[] = {

	// ASCII for "Server Key"
	0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x4B, 0x65, 0x79
};

static const unsigned char ClientKeyStr[] = {

	// ASCII for "Client Key"
	0x43, 0x6C, 0x69, 0x65, 0x6E, 0x74, 0x20, 0x4B, 0x65, 0x79
};

/*
 * For modules/saslserv/scram-sha to make an inter-module function call to
 * modules/crypto/pbkdf2v2:atheme_pbkdf2v2_scram_ex()
 */
typedef bool (*atheme_pbkdf2v2_scram_ex_fn)(const char *restrict, struct pbkdf2v2_parameters *restrict);

#endif /* !PBKDF2V2_H */
