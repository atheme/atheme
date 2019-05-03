/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Raw SHA2-512 password encryption.
 * Hash functions are not designed to encrypt passwords directly.
 */

#include <atheme.h>

#define MODULE_PREFIX_STR       "$rawsha512$"
#define MODULE_PREFIX_LEN       11
#define MODULE_DIGEST_LEN       DIGEST_MDLEN_SHA2_512
#define MODULE_PARAMS_LEN       (MODULE_PREFIX_LEN + (2 * MODULE_DIGEST_LEN))

static bool ATHEME_FATTR_WUR
atheme_rawsha512_verify(const char *const restrict password, const char *const restrict parameters,
                        unsigned int *const restrict flags)
{
	if (strlen(parameters) != MODULE_PARAMS_LEN)
		return false;

	if (strncmp(parameters, MODULE_PREFIX_STR, MODULE_PREFIX_LEN) != 0)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	unsigned char digest[MODULE_DIGEST_LEN];

	if (! digest_oneshot(DIGALG_SHA2_512, password, strlen(password), digest, NULL))
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

static const struct crypt_impl crypto_rawsha512_impl = {

	.id         = "rawsha512",
	.verify     = &atheme_rawsha512_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_rawsha512_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_rawsha512_impl);
}

SIMPLE_DECLARE_MODULE_V1("crypto/rawsha512", MODULE_UNLOAD_CAPABILITY_OK)
