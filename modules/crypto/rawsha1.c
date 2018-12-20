/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Raw SHA1 password encryption, as used by e.g. Anope 1.8.
 * Hash functions are not designed to encrypt passwords directly,
 * but we need this to convert some Anope databases.
 */

#include "atheme.h"

#define MODULE_PREFIX_STR       "$rawsha1$"
#define MODULE_PREFIX_LEN       9
#define MODULE_DIGEST_LEN       DIGEST_MDLEN_SHA1
#define MODULE_PARAMS_LEN       (MODULE_PREFIX_LEN + (2 * MODULE_DIGEST_LEN))

static bool
atheme_rawsha1_verify(const char *const restrict password, const char *const restrict parameters,
                      unsigned int *const restrict flags)
{
	if (strlen(parameters) != MODULE_PARAMS_LEN)
		return false;

	if (strncmp(parameters, MODULE_PREFIX_STR, MODULE_PREFIX_LEN) != 0)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	unsigned char digest[MODULE_DIGEST_LEN];

	if (! digest_oneshot(DIGALG_SHA1, password, strlen(password), digest, NULL))
		return false;

	char result[(2 * MODULE_DIGEST_LEN) + 1];

	for (size_t i = 0; i < sizeof digest; i++)
		(void) sprintf(result + i * 2, "%02x", 255 & digest[i]);

	if (strcmp(result, parameters + MODULE_PREFIX_LEN) != 0)
		return false;

	(void) smemzero(digest, sizeof digest);
	(void) smemzero(result, sizeof result);

	return true;
}

static const struct crypt_impl crypto_rawsha1_impl = {

	.id         = "rawsha1",
	.verify     = &atheme_rawsha1_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_rawsha1_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_rawsha1_impl);
}

SIMPLE_DECLARE_MODULE_V1("crypto/rawsha1", MODULE_UNLOAD_CAPABILITY_OK)
