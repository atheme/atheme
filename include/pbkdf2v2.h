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
	const EVP_MD    *md;
	unsigned char    cdg[EVP_MAX_MD_SIZE];
	unsigned char    sdg[EVP_MAX_MD_SIZE];
	unsigned char    ssk[EVP_MAX_MD_SIZE];
	unsigned char    shk[EVP_MAX_MD_SIZE];
	char             salt[0x2000];
	size_t           dl;
	size_t           sl;
	unsigned int     a;
	unsigned int     c;
	bool             scram;
};

#endif /* !PBKDF2V2_H */
