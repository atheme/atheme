/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2017-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>

#define ATHEME_PBKDF2_ROUNDS    128000U
#define ATHEME_PBKDF2_SALTLEN   16U
#define ATHEME_PBKDF2_PARAMLEN  (ATHEME_PBKDF2_SALTLEN + (2 * DIGEST_MDLEN_SHA2_512))

static bool ATHEME_FATTR_WUR
atheme_pbkdf2_verify(const char *const restrict password, const char *const restrict parameters,
                     unsigned int ATHEME_VATTR_UNUSED *const restrict flags)
{
	if (strlen(parameters) != ATHEME_PBKDF2_PARAMLEN)
		return false;

	for (size_t i = 0; i < ATHEME_PBKDF2_PARAMLEN; i++)
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
	                                             ATHEME_PBKDF2_SALTLEN, ATHEME_PBKDF2_ROUNDS, rawdigest,
	                                             sizeof rawdigest);
	if (! digest_ok)
		return false;

	for (size_t i = 0; i < sizeof rawdigest; i++)
		(void) sprintf(hexdigest + (i * 2), "%02x", 255 & rawdigest[i]);

	const int ret = smemcmp(hexdigest, parameters + ATHEME_PBKDF2_SALTLEN, sizeof hexdigest);

	(void) smemzero(rawdigest, sizeof rawdigest);
	(void) smemzero(hexdigest, sizeof hexdigest);

	return (ret == 0);
}

static const struct crypt_impl crypto_pbkdf2_impl = {

	.id         = "pbkdf2",
	.verify     = &atheme_pbkdf2_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_pbkdf2_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_pbkdf2_impl);
}

SIMPLE_DECLARE_MODULE_V1("crypto/pbkdf2", MODULE_UNLOAD_CAPABILITY_OK)
