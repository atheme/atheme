/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020-2021 Atheme Development Group (https://atheme.github.io/)
 */

#ifndef ATHEME_MOD_STATSERV_PWHASHES_H
#define ATHEME_MOD_STATSERV_PWHASHES_H 1

#include <atheme.h>
#include <atheme/pbkdf2.h>

#define SCANFMT_BASE64_CRYPT3           "%[" BASE64_ALPHABET_CRYPT3 "]"
#define SCANFMT_BASE64_RFC4648          "%[" BASE64_ALPHABET_RFC4648 "]"
#define SCANFMT_HEXADECIMAL             "%[A-Fa-f0-9]"

#define SCANFMT_ANOPE_ENC_SHA256        "$anope$enc_sha256$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_ARGON2                  "$%[A-Za-z0-9]$v=%u$m=%u,t=%u,p=%u$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_BASE64                  "$base64$" SCANFMT_BASE64_RFC4648
#define SCANFMT_BCRYPT                  "$2%1[ab]$%2u$%22[" BASE64_ALPHABET_CRYPT3 "]%31[" BASE64_ALPHABET_CRYPT3 "]"
#define SCANFMT_CRYPT3_DES              SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_MD5              "$1$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_256         "$5$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_256_EXT     "$5$rounds=%u$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_512         "$6$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_512_EXT     "$6$rounds=%u$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_IRCSERVICES             "$ircservices$" SCANFMT_HEXADECIMAL
#define SCANFMT_PBKDF2                  "%16[A-Za-z0-9]%128[A-Fa-f0-9]"
#define SCANFMT_PBKDF2V2_SCRAM          "$z$%u$%u$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_PBKDF2V2_HMAC           "$z$%u$%u$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_RAWMD5                  "$rawmd5$" SCANFMT_HEXADECIMAL
#define SCANFMT_RAWSHA1                 "$rawsha1$" SCANFMT_HEXADECIMAL
#define SCANFMT_RAWSHA2_256             "$rawsha256$" SCANFMT_HEXADECIMAL
#define SCANFMT_RAWSHA2_512             "$rawsha512$" SCANFMT_HEXADECIMAL
#define SCANFMT_SCRYPT                  "$scrypt$ln=%u,r=%u,p=%u$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3

#define METADATA_PWHASH_TOKEN_PDNAME    "private:statserv:pwhash:token"

enum pwhash_type
{
	PWHASH_TYPE_NONE = 0,
	PWHASH_TYPE_UNKNOWN,
	PWHASH_TYPE_ANOPE_ENC_SHA256,
	PWHASH_TYPE_ARGON2D,
	PWHASH_TYPE_ARGON2I,
	PWHASH_TYPE_ARGON2ID,
	PWHASH_TYPE_BASE64,
	PWHASH_TYPE_BCRYPT,
	PWHASH_TYPE_CRYPT3_DES,
	PWHASH_TYPE_CRYPT3_MD5,
	PWHASH_TYPE_CRYPT3_SHA2_256,
	PWHASH_TYPE_CRYPT3_SHA2_512,
	PWHASH_TYPE_IRCSERVICES,
	PWHASH_TYPE_PBKDF2,
	PWHASH_TYPE_PBKDF2V2_HMAC_MD5,
	PWHASH_TYPE_PBKDF2V2_HMAC_SHA1,
	PWHASH_TYPE_PBKDF2V2_HMAC_SHA2_256,
	PWHASH_TYPE_PBKDF2V2_HMAC_SHA2_512,
	PWHASH_TYPE_PBKDF2V2_SCRAM_MD5,
	PWHASH_TYPE_PBKDF2V2_SCRAM_SHA1,
	PWHASH_TYPE_PBKDF2V2_SCRAM_SHA2_256,
	PWHASH_TYPE_PBKDF2V2_SCRAM_SHA2_512,
	PWHASH_TYPE_RAWMD5,
	PWHASH_TYPE_RAWSHA1,
	PWHASH_TYPE_RAWSHA2_256,
	PWHASH_TYPE_RAWSHA2_512,
	PWHASH_TYPE_SCRYPT,
	PWHASH_TYPE_TOTAL_COUNT,
};

static char *pwhash_type_to_token[PWHASH_TYPE_TOTAL_COUNT] = {
	"none",
	"unknown",
	"anope_sha2_256",
	"argon2d",
	"argon2i",
	"argon2id",
	"base64",
	"bcrypt",
	"crypt3_des",
	"crypt3_md5",
	"crypt3_sha2_256",
	"crypt3_sha2_512",
	"ircservices",
	"pbkdf2",
	"pbkdf2v2_hmac_md5",
	"pbkdf2v2_hmac_sha1",
	"pbkdf2v2_hmac_sha2_256",
	"pbkdf2v2_hmac_sha2_512",
	"pbkdf2v2_scram_md5",
	"pbkdf2v2_scram_sha1",
	"pbkdf2v2_scram_sha2_256",
	"pbkdf2v2_scram_sha2_512",
	"md5",
	"sha1",
	"sha2_256",
	"sha2_512",
	"scrypt",
};

static char *pwhash_type_to_modname[PWHASH_TYPE_TOTAL_COUNT] = {
	"none",
	"unknown",
	"crypto/anope-enc-sha256",
	"crypto/argon2",
	"crypto/argon2",
	"crypto/argon2",
	"crypto/base64",
	"crypto/bcrypt",
	"crypto/crypt3-des",
	"crypto/crypt3-md5",
	"crypto/crypt3-sha2-256",
	"crypto/crypt3-sha2-512",
	"crypto/ircservices",
	"crypto/pbkdf2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/pbkdf2v2",
	"crypto/rawmd5",
	"crypto/rawsha1",
	"crypto/rawsha2-256",
	"crypto/rawsha2-512",
	"crypto/sodium-scrypt",
};

static char *pwhash_type_to_algorithm[PWHASH_TYPE_TOTAL_COUNT] = {
	"PLAIN-TEXT!",
	"UNKNOWN",
	"SHA2-256",
	"Argon2d",
	"Argon2i",
	"Argon2id",
	"PLAIN-TEXT!",
	"EksBlowfish",
	"DES",
	"MD5",
	"SHA2-256",
	"SHA2-512",
	"MD5",
	"HMAC-SHA2-512",
	"HMAC-MD5",
	"HMAC-SHA1",
	"HMAC-SHA2-256",
	"HMAC-SHA2-512",
	"SCRAM-MD5",
	"SCRAM-SHA-1",
	"SCRAM-SHA-256",
	"SCRAM-SHA-512",
	"MD5",
	"SHA1",
	"SHA2-256",
	"SHA2-512",
	"scrypt",
};

static enum pwhash_type
pwhash_compute_type(struct myuser *const restrict mu)
{
	char s1[BUFSIZE];
	char s2[BUFSIZE];
	char s3[BUFSIZE];
	char s4[BUFSIZE];
	unsigned int i1;
	unsigned int i2;
	unsigned int i3;
	unsigned int i4;

	const char *const pwstr = mu->pass;
	const size_t pwlen = strlen(pwstr);

	enum pwhash_type result = PWHASH_TYPE_UNKNOWN;

	if (! (mu->flags & MU_CRYPTPASS))
	{
		result = PWHASH_TYPE_NONE;
	}
	else if (sscanf(pwstr, SCANFMT_ANOPE_ENC_SHA256, s1, s2) == 2)
	{
		result = PWHASH_TYPE_ANOPE_ENC_SHA256;
	}
	else if (sscanf(pwstr, SCANFMT_ARGON2, s1, &i1, &i2, &i3, &i4, s2, s3) == 7)
	{
		if (strcasecmp(s1, "argon2d") == 0)
			result = PWHASH_TYPE_ARGON2D;
		else if (strcasecmp(s1, "argon2i") == 0)
			result = PWHASH_TYPE_ARGON2I;
		else if (strcasecmp(s1, "argon2id") == 0)
			result = PWHASH_TYPE_ARGON2ID;
		else
			result = PWHASH_TYPE_UNKNOWN;
	}
	else if (sscanf(pwstr, SCANFMT_BASE64, s1) == 1)
	{
		result = PWHASH_TYPE_BASE64;
	}
	else if (pwlen >= 60U && sscanf(pwstr, SCANFMT_BCRYPT, s1, &i1, s2, s3) == 4)
	{
		result = PWHASH_TYPE_BCRYPT;
	}
	else if (pwlen == 13U && sscanf(pwstr, SCANFMT_CRYPT3_DES, s1) == 1 && strcmp(s1, pwstr) == 0)
	{
		// Fuzzy (no rigid format)
		result = PWHASH_TYPE_CRYPT3_DES;
	}
	else if (sscanf(pwstr, SCANFMT_CRYPT3_MD5, s1, s2) == 2)
	{
		result = PWHASH_TYPE_CRYPT3_MD5;
	}
	else if (sscanf(pwstr, SCANFMT_CRYPT3_SHA2_256, s1, s2) == 2)
	{
		result = PWHASH_TYPE_CRYPT3_SHA2_256;
	}
	else if (sscanf(pwstr, SCANFMT_CRYPT3_SHA2_256_EXT, &i1, s1, s2) == 3)
	{
		result = PWHASH_TYPE_CRYPT3_SHA2_256;
	}
	else if (sscanf(pwstr, SCANFMT_CRYPT3_SHA2_512, s1, s2) == 2)
	{
		result = PWHASH_TYPE_CRYPT3_SHA2_512;
	}
	else if (sscanf(pwstr, SCANFMT_CRYPT3_SHA2_512_EXT, &i1, s1, s2) == 3)
	{
		result = PWHASH_TYPE_CRYPT3_SHA2_512;
	}
	else if (sscanf(pwstr, SCANFMT_IRCSERVICES, s1) == 1)
	{
		result = PWHASH_TYPE_IRCSERVICES;
	}
	else if (pwlen == 144U && sscanf(pwstr, SCANFMT_PBKDF2, s1, s2) == 2 &&
	         strlen(s1) == 16U && strlen(s2) == 128U)
	{
		// Fuzzy (no rigid format)
		result = PWHASH_TYPE_PBKDF2;
	}
	else if (sscanf(pwstr, SCANFMT_PBKDF2V2_SCRAM, &i1, &i2, s1, s2, s3) == 5)
	{
		switch (i1)
		{
			case PBKDF2_PRF_SCRAM_MD5:
			case PBKDF2_PRF_SCRAM_MD5_S64:
				result = PWHASH_TYPE_PBKDF2V2_SCRAM_MD5;
				break;
			case PBKDF2_PRF_SCRAM_SHA1:
			case PBKDF2_PRF_SCRAM_SHA1_S64:
				result = PWHASH_TYPE_PBKDF2V2_SCRAM_SHA1;
				break;
			case PBKDF2_PRF_SCRAM_SHA2_256:
			case PBKDF2_PRF_SCRAM_SHA2_256_S64:
				result = PWHASH_TYPE_PBKDF2V2_SCRAM_SHA2_256;
				break;
			case PBKDF2_PRF_SCRAM_SHA2_512:
			case PBKDF2_PRF_SCRAM_SHA2_512_S64:
				result = PWHASH_TYPE_PBKDF2V2_SCRAM_SHA2_512;
				break;
			default:
				result = PWHASH_TYPE_UNKNOWN;
				break;
		}
	}
	else if (sscanf(pwstr, SCANFMT_PBKDF2V2_HMAC, &i1, &i2, s1, s2) == 4)
	{
		switch (i1)
		{
			case PBKDF2_PRF_HMAC_MD5:
			case PBKDF2_PRF_HMAC_MD5_S64:
				result = PWHASH_TYPE_PBKDF2V2_HMAC_MD5;
				break;
			case PBKDF2_PRF_HMAC_SHA1:
			case PBKDF2_PRF_HMAC_SHA1_S64:
				result = PWHASH_TYPE_PBKDF2V2_HMAC_SHA1;
				break;
			case PBKDF2_PRF_HMAC_SHA2_256:
			case PBKDF2_PRF_HMAC_SHA2_256_S64:
				result = PWHASH_TYPE_PBKDF2V2_HMAC_SHA2_256;
				break;
			case PBKDF2_PRF_HMAC_SHA2_512:
			case PBKDF2_PRF_HMAC_SHA2_512_S64:
				result = PWHASH_TYPE_PBKDF2V2_HMAC_SHA2_512;
				break;
			default:
				result = PWHASH_TYPE_UNKNOWN;
				break;
		}
	}
	else if (sscanf(pwstr, SCANFMT_RAWMD5, s1) == 1)
	{
		result = PWHASH_TYPE_RAWMD5;
	}
	else if (sscanf(pwstr, SCANFMT_RAWSHA1, s1) == 1)
	{
		result = PWHASH_TYPE_RAWSHA1;
	}
	else if (sscanf(pwstr, SCANFMT_RAWSHA2_256, s1) == 1)
	{
		result = PWHASH_TYPE_RAWSHA2_256;
	}
	else if (sscanf(pwstr, SCANFMT_RAWSHA2_512, s1) == 1)
	{
		result = PWHASH_TYPE_RAWSHA2_512;
	}
	else if (sscanf(pwstr, SCANFMT_SCRYPT, &i1, &i2, &i3, s1, s2) == 5)
	{
		result = PWHASH_TYPE_SCRYPT;
	}

	(void) smemzero(s1, sizeof s1);
	(void) smemzero(s2, sizeof s2);
	(void) smemzero(s3, sizeof s3);
	(void) smemzero(s4, sizeof s4);

	(void) privatedata_set(mu, METADATA_PWHASH_TOKEN_PDNAME, pwhash_type_to_token[result]);

	return result;
}

static enum pwhash_type
pwhash_get_type(struct myuser *const restrict mu)
{
	const char *const token = privatedata_get(mu, METADATA_PWHASH_TOKEN_PDNAME);

	if (! token)
		return pwhash_compute_type(mu);

	for (enum pwhash_type i = PWHASH_TYPE_NONE; i < PWHASH_TYPE_TOTAL_COUNT; i++)
		if (strcmp(pwhash_type_to_token[i], token) == 0)
			return i;

	return PWHASH_TYPE_UNKNOWN;
}

static void
pwhash_invalidate_user_token(struct myuser *const restrict mu)
{
	(void) privatedata_delete(mu, METADATA_PWHASH_TOKEN_PDNAME);
}

static void
pwhash_invalidate_token_cache(void)
{
	struct myentity *mt;
	struct myentity_iteration_state state;
	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		struct myuser *const mu = user(mt);

		(void) pwhash_invalidate_user_token(mu);
	}
}

#endif /* !ATHEME_MOD_STATSERV_PWHASHES_H */
