/*
 * Copyright (C) 2015-2017 Aaron Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#ifdef HAVE_LIBIDN
#include <stringprep.h>
#endif /* HAVE_LIBIDN */

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#define PBKDF2_FN_PREFIX                "$z$%u$%u$"
#define PBKDF2_FN_BASE64                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="

#define PBKDF2_FN_LOADSALT              PBKDF2_FN_PREFIX "%[" PBKDF2_FN_BASE64 "]$"
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

static const unsigned char ServerKeyStr[] = {

	// ASCII for "Server Key"
	0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x4B, 0x65, 0x79
};

static const unsigned char ClientKeyStr[] = {

	// ASCII for "Client Key"
	0x43, 0x6C, 0x69, 0x65, 0x6E, 0x74, 0x20, 0x4B, 0x65, 0x79
};

static unsigned int pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;
static unsigned int pbkdf2v2_rounds = PBKDF2_ITERCNT_DEF;
static unsigned int pbkdf2v2_saltsz = PBKDF2_SALTLEN_DEF;

static const char *
atheme_pbkdf2v2_salt(void)
{
	unsigned char salt[PBKDF2_SALTLEN_MAX];

	(void) arc4random_buf(salt, pbkdf2v2_saltsz);

	char salt64[PBKDF2_SALTLEN_MAX * 3];

	if (base64_encode(salt, pbkdf2v2_saltsz, salt64, sizeof salt64) == (size_t) -1)
	{
		(void) slog(LG_ERROR, "%s: base64_encode() failed (BUG)", __func__);
		return NULL;
	}

	static char res[PASSLEN];

	if (snprintf(res, PASSLEN, PBKDF2_FN_SAVESALT, pbkdf2v2_digest, pbkdf2v2_rounds, salt64) >= PASSLEN)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
		return NULL;
	}

	return res;
}

static const char *
atheme_pbkdf2v2_crypt(const char *const restrict password, const char *const restrict parameters)
{
	unsigned int prf;
	unsigned int c;
	char salt64[0x1000];

	if (sscanf(parameters, PBKDF2_FN_LOADSALT, &prf, &c, salt64) != 3)
	{
		(void) slog(LG_DEBUG, "%s: sscanf(3) was unsuccessful", __func__);
		return NULL;
	}

	const EVP_MD *md = NULL;
	size_t hashlen = 0;
	bool scram = false;
	bool salt_was_b64 = false;

	switch (prf)
	{
		case PBKDF2_PRF_SCRAM_SHA1:
		case PBKDF2_PRF_SCRAM_SHA1_S64:
			scram = true;
			/* FALLTHROUGH */
		case PBKDF2_PRF_HMAC_SHA1:
		case PBKDF2_PRF_HMAC_SHA1_S64:
			md = EVP_sha1();
			hashlen = SHA_DIGEST_LENGTH;
			break;

		case PBKDF2_PRF_SCRAM_SHA2_256:
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
			scram = true;
			/* FALLTHROUGH */
		case PBKDF2_PRF_HMAC_SHA2_256:
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
			md = EVP_sha256();
			hashlen = SHA256_DIGEST_LENGTH;
			break;

		case PBKDF2_PRF_SCRAM_SHA2_512:
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			scram = true;
			/* FALLTHROUGH */
		case PBKDF2_PRF_HMAC_SHA2_512:
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
			md = EVP_sha512();
			hashlen = SHA512_DIGEST_LENGTH;
			break;

		default:
			(void) slog(LG_DEBUG, "%s: PRF ID '%u' unknown", __func__, prf);
			return NULL;
	}
	switch (prf)
	{
		case PBKDF2_PRF_HMAC_SHA1_S64:
		case PBKDF2_PRF_HMAC_SHA2_256_S64:
		case PBKDF2_PRF_HMAC_SHA2_512_S64:
		case PBKDF2_PRF_SCRAM_SHA1_S64:
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			salt_was_b64 = true;
			break;
	}
	if (! md)
	{
		(void) slog(LG_ERROR, "%s: md is NULL", __func__);
		return NULL;
	}
	if (c < PBKDF2_ITERCNT_MIN || c > PBKDF2_ITERCNT_MAX)
	{
		(void) slog(LG_ERROR, "%s: iteration count '%u' out of range", __func__, c);
		return NULL;
	}

	unsigned char salt[PBKDF2_SALTLEN_MAX];
	size_t saltlen;

	if (salt_was_b64)
	{
		if ((saltlen = base64_decode(salt64, salt, sizeof salt)) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_decode('%s') for salt failed", __func__, salt64);
			return NULL;
		}
		if (saltlen < PBKDF2_SALTLEN_MIN || saltlen > PBKDF2_SALTLEN_MAX)
		{
			(void) slog(LG_ERROR, "%s: salt length '%zu' out of range", __func__, saltlen);
			return NULL;
		}
	}
	else
	{
		saltlen = strlen(salt64);

		if (saltlen < PBKDF2_SALTLEN_MIN || saltlen > PBKDF2_SALTLEN_MAX)
		{
			(void) slog(LG_ERROR, "%s: salt length '%zu' out of range", __func__, saltlen);
			return NULL;
		}

		(void) memcpy(salt, salt64, saltlen);
	}

	char key[0x1000];
	(void) mowgli_strlcpy(key, password, sizeof key);

	if (scram)
	{
#ifdef HAVE_LIBIDN
		const int ret = stringprep(key, sizeof key, (Stringprep_profile_flags) 0, stringprep_saslprep);

		if (ret != STRINGPREP_OK)
		{
			(void) slog(LG_DEBUG, "%s: %s", __func__, stringprep_strerror((Stringprep_rc) ret));
			(void) explicit_bzero(key, sizeof key);
			return NULL;
		}
#else /* HAVE_LIBIDN */
		(void) slog(LG_INFO, "%s: encountered SCRAM format hash, but GNU libidn is unavailable", __func__);
		(void) slog(LG_INFO, "%s: user logins may fail if they have exotic password characters", __func__);
#endif /* !HAVE_LIBIDN */
	}

	const size_t keylen = strlen(key);
	if (! keylen)
	{
		(void) slog(LG_ERROR, "%s: password length == 0", __func__);
		(void) explicit_bzero(key, sizeof key);
		return NULL;
	}

	unsigned char cdg[EVP_MAX_MD_SIZE];
	if (PKCS5_PBKDF2_HMAC(key, (int) keylen, salt, (int) saltlen, (int) c, md, (int) hashlen, cdg) != 1)
	{
		(void) slog(LG_ERROR, "%s: PKCS5_PBKDF2_HMAC() failed", __func__);
		(void) explicit_bzero(key, sizeof key);
		return NULL;
	}

	(void) explicit_bzero(key, sizeof key);

	if (! salt_was_b64)
	{
		(void) memset(salt64, 0x00, sizeof salt64);
		(void) memcpy(salt64, salt, saltlen);
	}

	static char res[PASSLEN];

	if (scram)
	{
		unsigned char csk[EVP_MAX_MD_SIZE];
		unsigned char cck[EVP_MAX_MD_SIZE];
		unsigned char chk[EVP_MAX_MD_SIZE];
		char csk64[EVP_MAX_MD_SIZE * 3];
		char chk64[EVP_MAX_MD_SIZE * 3];

		if (! HMAC(md, cdg, (int) hashlen, ServerKeyStr, sizeof ServerKeyStr, csk, NULL))
		{
			(void) slog(LG_ERROR, "%s: HMAC() failed for csk", __func__);
			return NULL;
		}
		if (! HMAC(md, cdg, (int) hashlen, ClientKeyStr, sizeof ClientKeyStr, cck, NULL))
		{
			(void) slog(LG_ERROR, "%s: HMAC() failed for cck", __func__);
			return NULL;
		}
		if (EVP_Digest(cck, hashlen, chk, NULL, md, NULL) != 1)
		{
			(void) slog(LG_ERROR, "%s: EVP_Digest(cck) failed for chk", __func__);
			return NULL;
		}
		if (base64_encode(csk, hashlen, csk64, sizeof csk64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for csk", __func__);
			return NULL;
		}
		if (base64_encode(chk, hashlen, chk64, sizeof chk64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for chk", __func__);
			return NULL;
		}
		if (snprintf(res, PASSLEN, PBKDF2_FS_SAVEHASH, prf, c, salt64, csk64, chk64) >= PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
			return NULL;
		}
	}
	else
	{
		char cdg64[EVP_MAX_MD_SIZE * 3];

		if (base64_encode(cdg, hashlen, cdg64, sizeof cdg64) == (size_t) -1)
		{
			(void) slog(LG_ERROR, "%s: base64_encode() failed for cdg", __func__);
			return NULL;
		}
		if (snprintf(res, PASSLEN, PBKDF2_FN_SAVEHASH, prf, c, salt64, cdg64) >= PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
			return NULL;
		}
	}

	return res;
}

static bool
atheme_pbkdf2v2_upgrade(const char *const restrict parameters)
{
	unsigned int prf;
	unsigned int c;
	size_t saltlen;
	char salt64[0x1000];

	if (sscanf(parameters, PBKDF2_FN_LOADSALT, &prf, &c, salt64) != 3)
	{
		(void) slog(LG_ERROR, "%s: sscanf(3) was unsuccessful (BUG)", __func__);
		return false;
	}
	if (prf != pbkdf2v2_digest)
	{
		(void) slog(LG_DEBUG, "%s: prf (%u) != default (%u)", __func__, prf, pbkdf2v2_digest);
		return true;
	}
	if (c != pbkdf2v2_rounds)
	{
		(void) slog(LG_DEBUG, "%s: rounds (%u) != default (%u)", __func__, c, pbkdf2v2_rounds);
		return true;
	}
	if ((saltlen = base64_decode(salt64, NULL, 0)) == (size_t) -1)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode('%s') failed", __func__, salt64);
		return false;
	}
	if (saltlen != pbkdf2v2_saltsz)
	{
		(void) slog(LG_DEBUG, "%s: saltlen (%zu) != default (%u)", __func__, saltlen, pbkdf2v2_saltsz);
		return true;
	}

	return false;
}

static int
c_ci_pbkdf2v2_digest(mowgli_config_file_entry_t *const restrict ce)
{
	if (! ce->vardata)
	{
		(void) conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (! strcasecmp(ce->vardata, "SHA1"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SHA256"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_256_S64;
	else if (! strcasecmp(ce->vardata, "SHA512"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_512_S64;
#ifdef HAVE_LIBIDN
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA-1"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA1_S64;
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA-256"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_256_S64;
/*	// No specification
	else if (! strcasecmp(ce->vardata, "SCRAM-SHA-512"))
		pbkdf2v2_digest = PBKDF2_PRF_SCRAM_SHA2_512_S64;
*/
#endif /* HAVE_LIBIDN */
	else
	{
		(void) conf_report_warning(ce, "invalid parameter for configuration option -- using default");
		pbkdf2v2_digest = PBKDF2_PRF_DEFAULT;
	}

	return 0;
}

static crypt_impl_t crypto_pbkdf2v2_impl = {

	.id                     = "pbkdf2v2",
	.salt                   = &atheme_pbkdf2v2_salt,
	.crypt                  = &atheme_pbkdf2v2_crypt,
	.needs_param_upgrade    = &atheme_pbkdf2v2_upgrade,
};

static mowgli_list_t pbkdf2v2_conf_table;

static void
crypto_pbkdf2v2_modinit(module_t __attribute__((unused)) *const restrict m)
{
	(void) crypt_register(&crypto_pbkdf2v2_impl);

	(void) add_subblock_top_conf("PBKDF2V2", &pbkdf2v2_conf_table);
	(void) add_conf_item("DIGEST", &pbkdf2v2_conf_table, c_ci_pbkdf2v2_digest);
	(void) add_uint_conf_item("ROUNDS", &pbkdf2v2_conf_table, 0, &pbkdf2v2_rounds,
	                          PBKDF2_ITERCNT_MIN, PBKDF2_ITERCNT_MAX, PBKDF2_ITERCNT_DEF);
	(void) add_uint_conf_item("SALTLEN", &pbkdf2v2_conf_table, 0, &pbkdf2v2_saltsz,
	                          PBKDF2_SALTLEN_MIN, PBKDF2_SALTLEN_MAX, PBKDF2_SALTLEN_DEF);
}

static void
crypto_pbkdf2v2_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) del_conf_item("DIGEST", &pbkdf2v2_conf_table);
	(void) del_conf_item("ROUNDS", &pbkdf2v2_conf_table);
	(void) del_conf_item("SALTLEN", &pbkdf2v2_conf_table);
	(void) del_top_conf("PBKDF2V2");

	(void) crypt_unregister(&crypto_pbkdf2v2_impl);
}

DECLARE_MODULE_V1("crypto/pbkdf2v2", false, crypto_pbkdf2v2_modinit, crypto_pbkdf2v2_moddeinit,
                  PACKAGE_VERSION, "Aaron Jones <aaronmdjones@gmail.com>");

#endif /* HAVE_OPENSSL */
