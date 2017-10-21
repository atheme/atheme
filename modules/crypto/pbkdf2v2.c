/*
 * Copyright (C) 2015 Aaron Jones <aaronmdjones@gmail.com>
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

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

/*
 * Do not change anything below this line unless you know what you are doing,
 * AND how it will (possibly) break backward-, forward-, or cross-compatibility
 *
 * In particular, the salt length SHOULD NEVER BE CHANGED. 128 bits is more than
 * sufficient.
 */

#define PBKDF2_FN_PREFIX            "$z$%u$%u$"
#define PBKDF2_FN_BASE62            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

#define PBKDF2_FN_LOADSALT          PBKDF2_FN_PREFIX "%[" PBKDF2_FN_BASE62 "]$"
#define PBKDF2_FN_SAVESALT          PBKDF2_FN_PREFIX "%s$"
#define PBKDF2_FN_SAVEHASH          PBKDF2_FN_SAVESALT "%s"

#define PBKDF2_PRF_HMAC_SHA1        4U
#define PBKDF2_PRF_HMAC_SHA2_256    5U
#define PBKDF2_PRF_HMAC_SHA2_512    6U

#define PBKDF2_DIGEST_MIN           SHA_DIGEST_LENGTH
#define PBKDF2_DIGEST_MAX           SHA512_DIGEST_LENGTH
#define PBKDF2_DIGEST_DEF           PBKDF2_PRF_HMAC_SHA2_512

#define PBKDF2_ITERCNT_MIN          10000U
#define PBKDF2_ITERCNT_MAX          5000000U
#define PBKDF2_ITERCNT_DEF          64000U

#define PBKDF2_SALTLEN_MIN          8U
#define PBKDF2_SALTLEN_MAX          32U
#define PBKDF2_SALTLEN_DEF          16U

struct pbkdf2v2_parameters
{
	const EVP_MD    *md;
	unsigned char    cdg[EVP_MAX_MD_SIZE];
	char             salt[0x8000];
	size_t           dl;
	size_t           sl;
	unsigned int     a;
	unsigned int     c;
};

static const char salt_chars[62] = PBKDF2_FN_BASE62;

static unsigned int pbkdf2v2_digest = PBKDF2_DIGEST_DEF;
static unsigned int pbkdf2v2_rounds = PBKDF2_ITERCNT_DEF;

static const char *
atheme_pbkdf2v2_salt(void)
{
	/* Fill salt array with random bytes */
	unsigned char rawsalt[PBKDF2_SALTLEN_DEF];
	(void) arc4random_buf(rawsalt, sizeof rawsalt);

	/* Use random byte as index into printable character array, turning it into a printable string */
	char salt[sizeof rawsalt + 1];
	for (size_t i = 0; i < sizeof rawsalt; i++)
		salt[i] = salt_chars[rawsalt[i] % sizeof salt_chars];

	/* NULL-terminate the string */
	salt[sizeof rawsalt] = 0x00;

	/* Format and return the result */
	static char res[PASSLEN];
	if (snprintf(res, PASSLEN, PBKDF2_FN_SAVESALT, pbkdf2v2_digest, pbkdf2v2_rounds, salt) >= PASSLEN)
		return NULL;

	return res;
}

static bool
atheme_pbkdf2v2_compute(const char *const restrict password, const char *const restrict parameters,
                        struct pbkdf2v2_parameters *const restrict parsed)
{
	(void) memset(parsed, 0x00, sizeof *parsed);

	if (sscanf(parameters, PBKDF2_FN_LOADSALT, &parsed->a, &parsed->c, parsed->salt) != 3)
		return false;

	switch (parsed->a)
	{
		case PBKDF2_PRF_HMAC_SHA1:
			parsed->md = EVP_sha1();
			break;

		case PBKDF2_PRF_HMAC_SHA2_256:
			parsed->md = EVP_sha256();
			break;

		case PBKDF2_PRF_HMAC_SHA2_512:
			parsed->md = EVP_sha512();
			break;

		default:
			break;
	}

	if (! parsed->md)
		return false;

	parsed->sl = strlen(parsed->salt);

	if (parsed->sl < PBKDF2_SALTLEN_MIN || parsed->sl > PBKDF2_SALTLEN_MAX)
		return false;

	if (parsed->c < PBKDF2_ITERCNT_MIN || parsed->c > PBKDF2_ITERCNT_MAX)
		return false;

	const int dl_i = EVP_MD_size(parsed->md);

	if (dl_i < PBKDF2_DIGEST_MIN || dl_i > PBKDF2_DIGEST_MAX)
		return false;

	parsed->dl = (size_t) dl_i;

	const size_t pl = strlen(password);

	if (! pl)
		return false;

	const int ret = PKCS5_PBKDF2_HMAC(password, (int) pl, (unsigned char *) parsed->salt, (int) parsed->sl,
	                                  (int) parsed->c, parsed->md, dl_i, parsed->cdg);

	return (ret == 1) ? true : false;
}

static const char *
atheme_pbkdf2v2_crypt(const char *const restrict password, const char *const restrict parameters)
{
	struct pbkdf2v2_parameters parsed;

	if (! atheme_pbkdf2v2_compute(password, parameters, &parsed))
		return NULL;

	char cdg64[EVP_MAX_MD_SIZE * 3];

	if (base64_encode((const char *) parsed.cdg, parsed.dl, cdg64, sizeof cdg64) == (size_t) -1)
		return NULL;

	static char res[PASSLEN];

	if (snprintf(res, PASSLEN, PBKDF2_FN_SAVEHASH, parsed.a, parsed.c, parsed.salt, cdg64) >= PASSLEN)
		return NULL;

	return res;
}

static bool
atheme_pbkdf2v2_recrypt(const char *const restrict parameters)
{
	unsigned int prf;
	unsigned int iter;
	char salt[0x8000];

	(void) memset(salt, 0x00, sizeof salt);

	if (sscanf(parameters, PBKDF2_FN_LOADSALT, &prf, &iter, salt) != 3)
		return false;

	if (prf != pbkdf2v2_digest)
		return true;

	if (iter != pbkdf2v2_rounds)
		return true;

	if (strlen(salt) != PBKDF2_SALTLEN_DEF)
		return true;

	return false;
}

static int
c_ci_pbkdf2v2_digest(mowgli_config_file_entry_t *const restrict ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (!strcasecmp(ce->vardata, "SHA1"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA1;
	else if (!strcasecmp(ce->vardata, "SHA256"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_256;
	else if (!strcasecmp(ce->vardata, "SHA512"))
		pbkdf2v2_digest = PBKDF2_PRF_HMAC_SHA2_512;
	else
		conf_report_warning(ce, "invalid parameter for configuration option");

	return 0;
}

static crypt_impl_t crypto_pbkdf2v2_impl = {

	.id         = "pbkdf2v2",
	.salt       = &atheme_pbkdf2v2_salt,
	.crypt      = &atheme_pbkdf2v2_crypt,
	.recrypt    = &atheme_pbkdf2v2_recrypt,
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
}

static void
crypto_pbkdf2v2_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) del_conf_item("DIGEST", &pbkdf2v2_conf_table);
	(void) del_conf_item("ROUNDS", &pbkdf2v2_conf_table);
	(void) del_top_conf("PBKDF2V2");

	(void) crypt_unregister(&crypto_pbkdf2v2_impl);
}

DECLARE_MODULE_V1("crypto/pbkdf2v2", false, crypto_pbkdf2v2_modinit, crypto_pbkdf2v2_moddeinit,
                  PACKAGE_VERSION, "Aaron Jones <aaronmdjones@gmail.com>");

#endif /* HAVE_OPENSSL */
