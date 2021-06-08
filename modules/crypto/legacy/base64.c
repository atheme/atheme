/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * Verify-only module for base64-encoded plaintext passwords, as used in
 * e.g. Anope v1.8+. Uses constant-time comparison and securely erases
 * plaintext password buffers when complete.
 */

#include <atheme.h>

#define CRYPTO_MODULE_NAME      "crypto/base64"

#define MODULE_PREFIX_STR       "$base64$"
#define MODULE_PREFIX_LEN       8U

static bool ATHEME_FATTR_WUR
atheme_crypto_base64_verify(const char *const restrict password, const char *const restrict parameters,
                            unsigned int *const restrict flags)
{
	const size_t paramlen = strlen(parameters);

	if (paramlen <= MODULE_PREFIX_LEN)
		return false;

	if (strncmp(parameters, MODULE_PREFIX_STR, MODULE_PREFIX_LEN) != 0)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	char buf1[BASE64_SIZE_STR(PASSLEN)];
	char buf2[BASE64_SIZE_STR(PASSLEN)];

	(void) memset(buf1, 0x00, sizeof buf1);
	(void) memset(buf2, 0x00, sizeof buf2);

	(void) memcpy(buf1, parameters + MODULE_PREFIX_LEN, paramlen - MODULE_PREFIX_LEN);

	if (base64_encode(password, strlen(password), buf2, sizeof buf2) == BASE64_FAIL)
	{
		(void) slog(LG_ERROR, "%s: base64_encode() failed (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	const int ret = smemcmp(buf1, buf2, BASE64_SIZE_RAW(PASSLEN));

	(void) smemzero(buf1, sizeof buf1);
	(void) smemzero(buf2, sizeof buf2);

	return (ret == 0);
}

static const struct crypt_impl crypto_base64_impl = {

	.id         = CRYPTO_MODULE_NAME,
	.verify     = &atheme_crypto_base64_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_base64_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_base64_impl);
}

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
