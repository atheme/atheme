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

#define RAWSHA1_PREFIX "$rawsha1$"

static const char *
atheme_rawsha1_crypt(const char *password, const char *parameters)
{
	static char output[2 * SHA_DIGEST_LENGTH + sizeof(RAWSHA1_PREFIX)];
	SHA_CTX ctx;
	unsigned char digest[SHA_DIGEST_LENGTH];
	int i;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, password, strlen(password));
	SHA1_Final(digest, &ctx);

	strcpy(output, RAWSHA1_PREFIX);
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		sprintf(output + sizeof(RAWSHA1_PREFIX) - 1 + i * 2, "%02x",
				255 & digest[i]);

	return output;
}

static crypt_impl_t crypto_rawsha1_impl = {

	.id         = "rawsha1",
	.crypt      = &atheme_rawsha1_crypt,
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
