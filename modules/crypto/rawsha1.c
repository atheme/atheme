/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Raw SHA1 password encryption, as used by e.g. Anope 1.8.
 * Hash functions are not designed to encrypt passwords directly,
 * but we need this to convert some Anope databases.
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/sha.h>

#define MODULE_PREFIX_STR       "$rawsha1$"
#define MODULE_PREFIX_LEN       9
#define MODULE_DIGEST_LEN       SHA_DIGEST_LENGTH
#define MODULE_PARAMS_LEN       (MODULE_PREFIX_LEN + (2 * MODULE_DIGEST_LEN))

static bool
atheme_rawsha1_verify(const char *const restrict password, const char *const restrict parameters)
{
	if (strlen(parameters) != MODULE_PARAMS_LEN)
		return false;

	if (strncmp(parameters, MODULE_PREFIX_STR, MODULE_PREFIX_LEN) != 0)
		return false;

	SHA_CTX ctx;
	unsigned char digest[MODULE_DIGEST_LEN];

	(void) SHA1_Init(&ctx);
	(void) SHA1_Update(&ctx, (const unsigned char *) password, strlen(password));
	(void) SHA1_Final(digest, &ctx);

	char result[(2 * MODULE_DIGEST_LEN) + 1];

	for (size_t i = 0; i < sizeof digest; i++)
		(void) sprintf(result + i * 2, "%02x", 255 & digest[i]);

	if (strcmp(result, parameters + MODULE_PREFIX_LEN) != 0)
		return false;

	return true;
}

static crypt_impl_t crypto_rawsha1_impl = {

	.id         = "rawsha1",
	.verify     = &atheme_rawsha1_verify,
};

static void
crypto_rawsha1_modinit(module_t __attribute__((unused)) *const restrict m)
{
	(void) crypt_register(&crypto_rawsha1_impl);
}

static void
crypto_rawsha1_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) crypt_unregister(&crypto_rawsha1_impl);
}

DECLARE_MODULE_V1("crypto/rawsha1", false, crypto_rawsha1_modinit, crypto_rawsha1_moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);

#endif /* HAVE_OPENSSL */
