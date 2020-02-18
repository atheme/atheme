/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 */

#include <atheme.h>

#define ATHEME_BCRYPT_LOADSALT_FORMAT   "$2%1[ab]$%2u$%22[" BASE64_ALPHABET_CRYPT3_BLOWFISH "]"
#define ATHEME_BCRYPT_LOADHASH_FORMAT   ATHEME_BCRYPT_LOADSALT_FORMAT "%32[" BASE64_ALPHABET_CRYPT3_BLOWFISH "]"
#define ATHEME_BCRYPT_PARAMLEN_MIN      60U // The fixed-length salt and output guarantees a minimum MCF length

// The algorithm generates a 24-byte result but almost every implementation only uses 23 of them
#define ATHEME_BCRYPT_HASHLEN_TRUNC     23U

#define ATHEME_BCRYPT_PREFIXLEN         7U
#define ATHEME_BCRYPT_SALTLEN_B64       22U // base64-encoded 16 bytes *without padding* == 22 bytes
#define ATHEME_BCRYPT_HASHLEN_B64       31U // base64-encoded 23 bytes *without padding* == 31 bytes

#define CRYPTO_MODULE_NAME              "crypto/bcrypt"

static mowgli_list_t **crypto_conf_table = NULL;

static unsigned int atheme_bcrypt_cost = ATHEME_BCRYPT_ROUNDS_DEF;

static const char *
atheme_bcrypt_crypt(const char *const restrict password, const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	static char result[PASSLEN + 1];
	unsigned char salt[ATHEME_BCRYPT_SALTLEN];
	unsigned char hash[ATHEME_BCRYPT_HASHLEN];

	(void) memset(result, 0x00, sizeof result);
	(void) atheme_random_buf(salt, sizeof salt);

	if (! atheme_eks_bf_compute(password, ATHEME_BCRYPT_VERSION_MINOR, atheme_bcrypt_cost, salt, hash))
	{
		(void) slog(LG_ERROR, "%s: atheme_eks_bf_compute() failed (BUG)", MOWGLI_FUNC_NAME);
		return NULL;
	}

	if (snprintf(result, sizeof result, "$2%c$%2.2u$", ATHEME_BCRYPT_VERSION_MINOR, atheme_bcrypt_cost) !=
	      ATHEME_BCRYPT_PREFIXLEN)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) did not write an acceptable amount of data (BUG)",
		                      MOWGLI_FUNC_NAME);
		(void) smemzero(hash, sizeof hash);
		return NULL;
	}

	if (base64_encode_table(salt, ATHEME_BCRYPT_SALTLEN, result + ATHEME_BCRYPT_PREFIXLEN,
	      ATHEME_BCRYPT_SALTLEN_B64, BASE64_ALPHABET_CRYPT3_BLOWFISH) != ATHEME_BCRYPT_SALTLEN_B64)
	{
		(void) slog(LG_ERROR, "%s: base64_encode_table(salt) failed (BUG)", MOWGLI_FUNC_NAME);
		(void) smemzero(hash, sizeof hash);
		return NULL;
	}

	// We only encode 23 of the 24 bytes for wider compatibility with other implementations
	if (base64_encode_table(hash, ATHEME_BCRYPT_HASHLEN_TRUNC, result + ATHEME_BCRYPT_PREFIXLEN +
	      ATHEME_BCRYPT_SALTLEN_B64, ATHEME_BCRYPT_HASHLEN_B64, BASE64_ALPHABET_CRYPT3_BLOWFISH) !=
	      ATHEME_BCRYPT_HASHLEN_B64)
	{
		(void) slog(LG_ERROR, "%s: base64_encode_table(hash) failed (BUG)", MOWGLI_FUNC_NAME);
		(void) smemzero(hash, sizeof hash);
		(void) smemzero(result, sizeof result);
		return NULL;
	}

	(void) smemzero(hash, sizeof hash);
	return result;
}

static bool ATHEME_FATTR_WUR
atheme_bcrypt_verify(const char *const restrict password, const char *const restrict parameters,
                     unsigned int *const restrict flags)
{
	char minor[2];
	unsigned int cost;
	char salt64[BUFSIZE];
	char hash64[BUFSIZE];

	unsigned char salt[ATHEME_BCRYPT_SALTLEN];
	unsigned char hash[ATHEME_BCRYPT_HASHLEN];
	unsigned char cmphash[ATHEME_BCRYPT_HASHLEN];

	bool retval = false;

	if (strlen(parameters) < ATHEME_BCRYPT_PARAMLEN_MIN)
	{
		(void) slog(LG_DEBUG, "%s: parameters are not the correct length", MOWGLI_FUNC_NAME);
		return false;
	}
	if (sscanf(parameters, ATHEME_BCRYPT_LOADHASH_FORMAT, minor, &cost, salt64, hash64) != 4)
	{
		(void) slog(LG_DEBUG, "%s: sscanf(3) was unsuccessful", MOWGLI_FUNC_NAME);
		goto cleanup;
	}
	if (cost < ATHEME_BCRYPT_ROUNDS_MIN || cost > ATHEME_BCRYPT_ROUNDS_MAX)
	{
		(void) slog(LG_DEBUG, "%s: cost parameter '%u' unacceptable", MOWGLI_FUNC_NAME, cost);
		goto cleanup;
	}
	if (base64_decode_table(salt64, salt, sizeof salt, BASE64_ALPHABET_CRYPT3_BLOWFISH) != ATHEME_BCRYPT_SALTLEN)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode_table(salt) failed", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	// We can verify 23-byte or 24-byte digests for wider compatibility with other implementations
	const size_t hashlen = base64_decode_table(hash64, hash, sizeof hash, BASE64_ALPHABET_CRYPT3_BLOWFISH);

	if (hashlen == BASE64_FAIL || hashlen < ATHEME_BCRYPT_HASHLEN_TRUNC)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode_table(hash) failed", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	*flags |= PWVERIFY_FLAG_MYMODULE;

	if (! atheme_eks_bf_compute(password, (unsigned int) minor[0], cost, salt, cmphash))
	{
		(void) slog(LG_ERROR, "%s: atheme_eks_bf_compute() failed (BUG)", MOWGLI_FUNC_NAME);
		goto cleanup;
	}
	if (smemcmp(hash, cmphash, hashlen) != 0)
	{
		(void) slog(LG_DEBUG, "%s: smemcmp() mismatch; incorrect password?", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	retval = true;

	(void) slog(LG_DEBUG, "%s: authentication successful", MOWGLI_FUNC_NAME);

	if (minor[0] != ATHEME_BCRYPT_VERSION_MINOR)
	{
		(void) slog(LG_DEBUG, "%s: minor version (%s) is not current (%c)", MOWGLI_FUNC_NAME, minor,
		                      ATHEME_BCRYPT_VERSION_MINOR);

		*flags |= PWVERIFY_FLAG_RECRYPT;
	}
	if (cost != atheme_bcrypt_cost)
	{
		(void) slog(LG_DEBUG, "%s: cost (%u) is not the default (%u)", MOWGLI_FUNC_NAME, cost,
		                      atheme_bcrypt_cost);

		*flags |= PWVERIFY_FLAG_RECRYPT;
	}

cleanup:
	(void) smemzero(hash, sizeof hash);
	(void) smemzero(hash64, sizeof hash64);
	(void) smemzero(cmphash, sizeof cmphash);

	return retval;
}

static const struct crypt_impl crypto_bcrypt_impl = {

	.id        = CRYPTO_MODULE_NAME,
	.crypt     = &atheme_bcrypt_crypt,
	.verify    = &atheme_bcrypt_verify,
};

static void
mod_init(struct module *const restrict m)
{
	if (! atheme_eks_bf_testsuite_run())
	{
		(void) slog(LG_ERROR, "%s: self-test failed (BUG)", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table")

	(void) add_uint_conf_item("bcrypt_cost", *crypto_conf_table, 0, &atheme_bcrypt_cost,
	                          ATHEME_BCRYPT_ROUNDS_MIN, ATHEME_BCRYPT_ROUNDS_MAX, ATHEME_BCRYPT_ROUNDS_DEF);

	(void) crypt_register(&crypto_bcrypt_impl);

	(void) slog(LG_INFO, "%s: WARNING: Passwords greater than 72 characters are truncated!", m->name);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("bcrypt_cost", *crypto_conf_table);

	(void) crypt_unregister(&crypto_bcrypt_impl);
}

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
