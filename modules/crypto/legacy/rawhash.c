/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2009 Atheme Project (http://atheme.org/)
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Code shared by all (verify-only) raw digest crypto modules.
 */

#ifndef RAWHASH_MODULE_NAME
#  error "Do not compile me directly; compile another raw*.c instead"
#endif

#include <atheme.h>

#define RAWHASH_PREFIX_STR      "$" RAWHASH_MODULE_NAME "$"

/* Any decent optimising compiler will turn the strlen(3) below
 * (and thus these 2 macros) into a compile-time constant.   --amdj
 */
#define RAWHASH_PREFIX_LEN      strlen(RAWHASH_PREFIX_STR)
#define RAWHASH_PARAMS_LEN      (RAWHASH_PREFIX_LEN + (2 * RAWHASH_DIGEST_LEN))

static bool ATHEME_FATTR_WUR
atheme_rawhash_verify(const char *const restrict password, const char *const restrict parameters,
                      unsigned int *const restrict flags)
{
	if (strlen(parameters) != RAWHASH_PARAMS_LEN)
		return false;

	if (strncmp(parameters, RAWHASH_PREFIX_STR, RAWHASH_PREFIX_LEN) != 0)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	unsigned char digest[RAWHASH_DIGEST_LEN];

	if (! digest_oneshot(RAWHASH_DIGEST_ALG, password, strlen(password), digest, NULL))
		return false;

	char result[BUFSIZE];

	(void) memcpy(result, RAWHASH_PREFIX_STR, RAWHASH_PREFIX_LEN);

	for (size_t i = 0; i < sizeof digest; i++)
		(void) sprintf(result + RAWHASH_PREFIX_LEN + (i * 2), "%02x", 255 & digest[i]);

	const int ret = smemcmp(result, parameters, RAWHASH_PARAMS_LEN);

	(void) smemzero(digest, sizeof digest);
	(void) smemzero(result, sizeof result);

	return (ret == 0);
}

static const struct crypt_impl crypto_rawhash_impl = {

	.id         = "crypto/" RAWHASH_MODULE_NAME,
	.verify     = &atheme_rawhash_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_rawhash_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_rawhash_impl);
}

SIMPLE_DECLARE_MODULE_V1("crypto/" RAWHASH_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
