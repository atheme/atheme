/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * Verify-only module for Anope v2.0+ "enc_sha256" password hashes. This is
 * a straight SHA2-256 invocation but with a custom initial hash value,
 * which sets this module apart from modules/crypto/rawsha2-256.
 *
 * It also means that we can't use our Digest API directly, because it will
 * usually be provided by a crypto library, which do not permit changing
 * the IHVs. So we use our own internal digest algorithm implementation
 * directly.
 *
 * Note that this module differs from the Anope one in significant ways:
 *
 * 1: Anope uses a prefix of "sha256:", while this module uses a prefix of
 *    "$anope$enc_sha256$".
 *
 * 2: The 2 values (message digest and IHV) are encoded in base-64 instead
 *    of Anope's hexadecimal.
 *
 * 3: Fields are separated with "$" instead of Anope's ":".
 *
 * 4: The order of the two fields is swapped; the IHV is first.
 *
 * It is the responsibility of a database migration program to perform
 * these adaptations.
 */

// Since we need to avoid the Digest API, we can't include <atheme.h>
#include <atheme/base64.h>      // base64_decode()
#include <atheme/crypto.h>      // crypt_register(), crypt_unregister(), struct crypt_impl
#include <atheme/memory.h>      // smemcmp(), smemzero()
#include <atheme/module.h>      // MODFLAG_*, MODULE_*, *DECLARE_MODULE_*(), struct module, ...
#include <atheme/stdheaders.h>  // Everything else
#include <atheme/tools.h>       // LG_ERROR, slog()

// Yes, this is a hack ...
#include <atheme/digest/internal.h>
#include "../../libathemecore/digest_be_sha2.c"

#define BASE64_ALPHABET         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
#define MODULE_HASH_FORMAT      "$anope$enc_sha256$%[" BASE64_ALPHABET "]$%[" BASE64_ALPHABET "]"

static bool ATHEME_FATTR_WUR
anope_enc_sha256_verify(const char *const restrict password, const char *const restrict parameters,
                        unsigned int *const restrict flags)
{
	char iv64[BUFSIZE];
	char md64[BUFSIZE];

	(void) memset(iv64, 0x00, sizeof iv64);
	(void) memset(md64, 0x00, sizeof md64);

	if (sscanf(parameters, MODULE_HASH_FORMAT, iv64, md64) != 2)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	uint32_t iv[DIGEST_IVLEN_SHA2_256];
	unsigned char md[DIGEST_MDLEN_SHA2_256];

	if (base64_decode(iv64, iv, sizeof iv) != sizeof iv)
	{
		(void) slog(LG_ERROR, "%s: base64_decode() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (base64_decode(md64, md, sizeof md) != sizeof md)
	{
		(void) slog(LG_ERROR, "%s: base64_decode() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}

	if (! digest_is_big_endian_sha2())
		for (size_t i = 0; i < DIGEST_IVLEN_SHA2_256; i++)
			SHA2_REVERSE32(iv[i], iv[i]);

	unsigned char out[DIGEST_MDLEN_SHA2_256];
	union digest_state ctx;

	(void) memset(&ctx, 0x00, sizeof ctx);
	(void) memcpy(ctx.sha2_256_ctx.state, iv, sizeof iv);
	(void) digest_update_sha2_256(&ctx, password, strlen(password));
	(void) digest_final_sha2_256(&ctx, out);

	const int ret = smemcmp(md, out, DIGEST_MDLEN_SHA2_256);

	(void) smemzero(md, sizeof md);
	(void) smemzero(out, sizeof out);

	return (ret == 0);
}

static const struct crypt_impl crypto_impl = {

	.id         = "anope-enc-sha256",
	.verify     = &anope_enc_sha256_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_impl);
}

SIMPLE_DECLARE_MODULE_V1("crypto/anope-enc-sha256", MODULE_UNLOAD_CAPABILITY_OK)
