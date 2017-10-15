/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Raw MD5 password encryption, as used by e.g. Anope 1.8.
 * Hash functions are not designed to encrypt passwords directly,
 * but we need this to convert some Anope databases.
 */

#include "atheme.h"

#define RAWMD5_PREFIX "$rawmd5$"

static const char *
atheme_rawmd5_crypt(const char *password, const char *parameters)
{
	static char output[2 * 16 + sizeof(RAWMD5_PREFIX)];
	md5_state_t ctx;
	unsigned char digest[16];
	int i;

	md5_init(&ctx);
	md5_append(&ctx, (const unsigned char *)password, strlen(password));
	md5_finish(&ctx, digest);

	strcpy(output, RAWMD5_PREFIX);
	for (i = 0; i < 16; i++)
		sprintf(output + sizeof(RAWMD5_PREFIX) - 1 + i * 2, "%02x",
				255 & digest[i]);

	return output;
}

static crypt_impl_t crypto_rawmd5_impl = {

	.id         = "rawmd5",
	.crypt      = &atheme_rawmd5_crypt,
};

static void
crypto_rawmd5_modinit(module_t __attribute__((unused)) *const restrict m)
{
	(void) crypt_register(&crypto_rawmd5_impl);
}

static void
crypto_rawmd5_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) crypt_unregister(&crypto_rawmd5_impl);
}

DECLARE_MODULE_V1("crypto/rawmd5", false, crypto_rawmd5_modinit, crypto_rawmd5_moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);
