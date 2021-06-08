/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2017-2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

#define CRYPTO_MODULE_NAME PBKDF2_LEGACY_MODULE_NAME

static void
atheme_pbkdf2_config_ready(void ATHEME_VATTR_UNUSED *const restrict unused)
{
	(void) slog(LG_INFO, "%s: this module has been superseded by %s; please see the contrib/pbkdf2-convert.pl "
	                     "script in the source directory.", CRYPTO_MODULE_NAME, PBKDF2V2_CRYPTO_MODULE_NAME);
}

static bool ATHEME_FATTR_WUR
atheme_pbkdf2_verify(const char *const restrict password, const char *const restrict parameters,
                     unsigned int ATHEME_VATTR_UNUSED *const restrict flags)
{
	if (strlen(parameters) != PBKDF2_LEGACY_PARAMLEN)
		return false;

	for (size_t i = 0; i < PBKDF2_LEGACY_SALTLEN; i++)
		if (! isalnum(parameters[i]))
			return false;

	for (size_t i = PBKDF2_LEGACY_SALTLEN; i < PBKDF2_LEGACY_PARAMLEN; i++)
		if (! isxdigit(parameters[i]))
			return false;

	/* Note that this module's output string (from previous versions of services)
	 * has no defined format, so we can't reliably use PWVERIFY_FLAG_MYMODULE here
	 * (as per doc/CRYPTO-API). The most we can do is the sanitising above, to save
	 * some CPU time sometimes. This is partly why crypto/pbkdf2v2 was created.
	 *   -- amdj
	 */

	unsigned char rawdigest[DIGEST_MDLEN_SHA2_512];
	char hexdigest[(2 * sizeof rawdigest) + 1];

	const bool digest_ok = digest_oneshot_pbkdf2(DIGALG_SHA2_512, password, strlen(password), parameters,
	                                             PBKDF2_LEGACY_SALTLEN, PBKDF2_LEGACY_ITERCNT, rawdigest,
	                                             sizeof rawdigest);
	if (! digest_ok)
		return false;

	for (size_t i = 0; i < sizeof rawdigest; i++)
		(void) sprintf(hexdigest + (i * 2), "%02x", 255 & rawdigest[i]);

	const int ret = smemcmp(hexdigest, parameters + PBKDF2_LEGACY_SALTLEN, sizeof hexdigest);

	(void) smemzero(rawdigest, sizeof rawdigest);
	(void) smemzero(hexdigest, sizeof hexdigest);

	return (ret == 0);
}

static const struct crypt_impl crypto_pbkdf2_impl = {

	.id         = CRYPTO_MODULE_NAME,
	.verify     = &atheme_pbkdf2_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) hook_add_config_ready(&atheme_pbkdf2_config_ready);

	(void) crypt_register(&crypto_pbkdf2_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_config_ready(&atheme_pbkdf2_config_ready);

	(void) crypt_unregister(&crypto_pbkdf2_impl);
}

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
