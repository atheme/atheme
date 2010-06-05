/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Raw MD5 password encryption, as used by e.g. Anope 1.8.
 * Hash functions are not designed to encrypt passwords directly,
 * but we need this to convert some Anope databases.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/rawmd5", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

#define RAWMD5_PREFIX "$rawmd5$"

static const char *rawmd5_crypt_string(const char *key, const char *salt)
{
	static char output[2 * 16 + sizeof(RAWMD5_PREFIX)];
	md5_state_t ctx;
	unsigned char digest[16];
	int i;

	md5_init(&ctx);
	md5_append(&ctx, (const unsigned char *)key, strlen(key));
	md5_finish(&ctx, digest);

	strcpy(output, RAWMD5_PREFIX);
	for (i = 0; i < 16; i++)
		sprintf(output + sizeof(RAWMD5_PREFIX) - 1 + i * 2, "%02x",
				255 & digest[i]);

	return output;
}

void _modinit(module_t *m)
{
	crypt_string = &rawmd5_crypt_string;

	crypto_module_loaded = true;
}

void _moddeinit(void)
{
	crypt_string = &generic_crypt_string;

	crypto_module_loaded = false;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
