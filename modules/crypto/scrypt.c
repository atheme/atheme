/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * scrypt password hashing module using libsodium (verify-only).
 */

#include <atheme.h>

#define CRYPTO_MODULE_NAME "crypto/scrypt"

#ifdef HAVE_LIBSODIUM_SCRYPT

#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>

static mowgli_list_t **crypto_conf_table = NULL;

static unsigned int atheme_scrypt_memlimit = ATHEME_SCRYPT_MEMLIMIT_DEF;
static unsigned int atheme_scrypt_opslimit = ATHEME_SCRYPT_OPSLIMIT_DEF;

static inline size_t
atheme_scrypt_calc_real_memlimit(void)
{
	/* libsodium's password hashing API takes memory limits in bytes, but we specify the memory limit
	 * as a power of 2, in KiB. This matches the configuration interface of libargon2, for consistency.
	 */
	return ((1ULL << (size_t) atheme_scrypt_memlimit) * 1024ULL);
}

static const char *
atheme_scrypt_crypt(const char *const restrict password, const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	static char result[PASSLEN + 1];

	const unsigned long long opslimit = atheme_scrypt_opslimit;
	const size_t memlimit = atheme_scrypt_calc_real_memlimit();

	(void) memset(result, 0x00, sizeof result);

	if (crypto_pwhash_scryptsalsa208sha256_str(result, password, strlen(password), opslimit, memlimit) != 0)
	{
		const int errsv = errno;

		(void) slog(LG_ERROR, "%s: encrypting failed (BUG? Incorrect configuration?)", CRYPTO_MODULE_NAME);
		(void) slog(LG_ERROR, "%s: possibly useful information: opslimit %u, memlimit %u (%zu bytes), "
		                      "ptrsize %zu, errno %d (%s)", CRYPTO_MODULE_NAME, atheme_scrypt_opslimit,
		                      atheme_scrypt_memlimit, memlimit, sizeof (void *), errsv, strerror(errsv));
		return NULL;
	}

	return result;
}

static bool ATHEME_FATTR_WUR
atheme_scrypt_verify(const char *const restrict password, const char *const restrict parameters,
                     unsigned int *const restrict flags)
{
	const unsigned long long opslimit = atheme_scrypt_opslimit;
	const size_t memlimit = atheme_scrypt_calc_real_memlimit();

	const int needs_rehash = crypto_pwhash_scryptsalsa208sha256_str_needs_rehash(parameters, opslimit, memlimit);

	if (needs_rehash == -1)
	{
		(void) slog(LG_DEBUG, "%s: parse failure (parameters don't match)", MOWGLI_FUNC_NAME);
		return false;
	}

	*flags |= PWVERIFY_FLAG_MYMODULE;

	if (crypto_pwhash_scryptsalsa208sha256_str_verify(parameters, password, strlen(password)) != 0)
	{
		(void) slog(LG_DEBUG, "%s: verifying failed (incorrect password?)", MOWGLI_FUNC_NAME);
		return false;
	}

	if (needs_rehash == 1)
		*flags |= PWVERIFY_FLAG_RECRYPT;

	return true;
}

static const struct crypt_impl crypto_scrypt_impl = {

	.id        = CRYPTO_MODULE_NAME,
	.crypt     = &atheme_scrypt_crypt,
	.verify    = &atheme_scrypt_verify,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table")

	(void) add_uint_conf_item("scrypt_memlimit", *crypto_conf_table, 0, &atheme_scrypt_memlimit,
	                          ATHEME_SCRYPT_MEMLIMIT_MIN, ATHEME_SCRYPT_MEMLIMIT_MAX, ATHEME_SCRYPT_MEMLIMIT_DEF);

	(void) add_uint_conf_item("scrypt_opslimit", *crypto_conf_table, 0, &atheme_scrypt_opslimit,
	                          ATHEME_SCRYPT_OPSLIMIT_MIN, ATHEME_SCRYPT_OPSLIMIT_MAX, ATHEME_SCRYPT_OPSLIMIT_DEF);

	(void) crypt_register(&crypto_scrypt_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("scrypt_memlimit", *crypto_conf_table);
	(void) del_conf_item("scrypt_opslimit", *crypto_conf_table);

	(void) crypt_unregister(&crypto_scrypt_impl);
}

#else /* HAVE_LIBSODIUM_SCRYPT */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires libsodium (scrypt pwhash) support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_LIBSODIUM_SCRYPT */

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
