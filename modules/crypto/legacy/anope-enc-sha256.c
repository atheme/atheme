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

#include <atheme.h>

#define CRYPTO_MODULE_NAME      "crypto/anope-enc-sha256"

#define MODULE_HASH_FORMAT      "$anope$enc_sha256$%[" BASE64_ALPHABET_RFC4648 "]$%[" BASE64_ALPHABET_RFC4648 "]"

static bool ATHEME_FATTR_WUR
anope_enc_sha256_verify(const char *const restrict password, const char *const restrict parameters,
                        unsigned int *const restrict flags)
{
	char iv64[BUFSIZE];
	char md64[BUFSIZE];

	if (sscanf(parameters, MODULE_HASH_FORMAT, iv64, md64) != 2)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	uint32_t iv[DIGEST_IVLEN_SHA2_256];
	unsigned char md[DIGEST_MDLEN_SHA2_256];

	if (base64_decode(iv64, iv, sizeof iv) != sizeof iv)
	{
		(void) slog(LG_ERROR, "%s: base64_decode(iv) failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (base64_decode(md64, md, sizeof md) != sizeof md)
	{
		(void) slog(LG_ERROR, "%s: base64_decode(md) failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}

	(void) smemzero(md64, sizeof md64);

	for (size_t i = 0; i < DIGEST_IVLEN_SHA2_256; i++)
		iv[i] = htonl(iv[i]);

	unsigned char out[DIGEST_MDLEN_SHA2_256];
	union digest_direct_ctx ctx;

	(void) memset(&ctx, 0x00, sizeof ctx);
	(void) memcpy(ctx.sha2_256.state, iv, sizeof iv);
	(void) digest_direct_update_sha2_256(&ctx, password, strlen(password));
	(void) digest_direct_final_sha2_256(&ctx, out);

	const int ret = smemcmp(md, out, DIGEST_MDLEN_SHA2_256);

	(void) smemzero(md, sizeof md);
	(void) smemzero(out, sizeof out);

	return (ret == 0);
}

static const struct crypt_impl crypto_impl = {

	.id         = CRYPTO_MODULE_NAME,
	.verify     = &anope_enc_sha256_verify,
};

static bool ATHEME_FATTR_WUR
mod_selftest(void)
{
	static const struct {
		const char *    password;
		const char *    parameters;
	} vectors[] = {
		{
			.password   = "jKOBBtCvr9CbulQyhyHy3lUEPoRp0WtX",
			.parameters = "$anope$enc_sha256$J9b6Xi0sRclnvcffBi5Zg0HOactnhdcuHZUBVlAeUj4=$"
			              "bp3WKwgM4WZW/44cq8jfMwO1UyezeLBlo+Hq9uyReEc=",
		},
		{
			.password   = "NfA8mh6UBERSqqpAuw7FrzrWGVW7zMChvk26rf9PVLOEGePko0oAjWyhEkxTgpj"
			              "LjG7YYpQHO8VivPUVmlasVGs04ysdEtsFHVXVWaXldKjcnaoiRefnNKU3ery62p"
			              "05wE7klMQNTcWBLGXqixYxZDD2Ra3MVfVPvXr5XjMZIViVx0L4287X8PpdDAGgT"
			              "iDe2GVn3fuCGxwJbg8dEdG9uMw6b3XSCeqqnNJ8odTeYa2BZj0a99quz6xo0tvx"
			              "Em4Z8dew8E6eE1vAYrgvJ0iVyGRagdlDtQKT",
			.parameters = "$anope$enc_sha256$UX4ZIVS4Det/iP3EZKcsDzD0fsAGKk3EeKuXghmYk8M=$"
			              "pb/k1xaMgJQe9QGoNVuHQ+uTdRK7bL1bIudMy/Zaqhc=",
		},
	};

	for (size_t vector = 0; vector < ((sizeof vectors) / (sizeof vectors[0])); vector++)
	{
		unsigned int flags = 0;

		if (! anope_enc_sha256_verify(vectors[vector].password, vectors[vector].parameters, &flags))
			return false;

		if (! (flags & PWVERIFY_FLAG_MYMODULE))
			return false;
	}

	return true;
}

static void
mod_init(struct module *const restrict m)
{
	if (! mod_selftest())
	{
		(void) slog(LG_ERROR, "%s: self-test failed (BUG?); refusing to load.", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) crypt_register(&crypto_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_impl);
}

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
