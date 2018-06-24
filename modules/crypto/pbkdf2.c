/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2017-2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "atheme.h"

#define ATHEME_PBKDF2_ROUNDS    128000
#define ATHEME_PBKDF2_SALTLEN   16

static bool
atheme_pbkdf2_verify(const char *const restrict password, const char *const restrict parameters,
                     unsigned int ATHEME_VATTR_UNUSED *const restrict flags)
{
	if (strlen(parameters) != (ATHEME_PBKDF2_SALTLEN + (2 * DIGEST_MDLEN_SHA2_512)))
		return false;

	unsigned char buf[DIGEST_MDLEN_SHA2_512];

	const bool ret = digest_pbkdf2_hmac(DIGALG_SHA2_512, password, strlen(password), parameters,
	                                    ATHEME_PBKDF2_SALTLEN, ATHEME_PBKDF2_ROUNDS, buf, sizeof buf);

	if (! ret)
		return false;

	char result[(2 * DIGEST_MDLEN_SHA2_512) + 1];

	for (size_t i = 0; i < DIGEST_MDLEN_SHA2_512; i++)
		(void) sprintf(result + (i * 2), "%02x", 255 & buf[i]);

	if (strcmp(result, parameters + ATHEME_PBKDF2_SALTLEN) != 0)
		return false;

	(void) smemzero(buf, sizeof buf);
	(void) smemzero(result, sizeof result);

	return true;
}

static const struct crypt_impl crypto_pbkdf2_impl = {

	.id         = "pbkdf2",
	.verify     = &atheme_pbkdf2_verify,
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	(void) crypt_register(&crypto_pbkdf2_impl);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_pbkdf2_impl);
}

SIMPLE_DECLARE_MODULE_V1("crypto/pbkdf2", MODULE_UNLOAD_CAPABILITY_OK)
