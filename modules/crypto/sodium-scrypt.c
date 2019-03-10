/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * scrypt password hashing module using libsodium (verify-only).
 */

#include <atheme.h>

#ifdef HAVE_LIBSODIUM_SCRYPT

#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>

static bool ATHEME_FATTR_WUR
atheme_sodium_scrypt_verify(const char *const restrict password, const char *const restrict parameters,
                            unsigned int *const restrict flags)
{
	/* We run this first, even though it will always need rehashing (by services, not us, because this is a
	 * verify-only module), because it allows us to skip actually invoking the password hashing routine below
	 * if the parameter string does not look correct, which helps save CPU time when verifying a password
	 * against a parameter string that was *not* produced by this algorithm.    -- amdj
	 */
	const unsigned long long int opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_MIN;
	const size_t memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_MIN;
	if (crypto_pwhash_scryptsalsa208sha256_str_needs_rehash(parameters, opslimit, memlimit) == -1)
	{
		(void) slog(LG_DEBUG, "%s: crypto_pwhash_scryptsalsa208sha256_str_needs_rehash() failed "
		                      "(parameters not produced by this algorithm?)", MOWGLI_FUNC_NAME);
		return false;
	}

	// We are now certain this parameter string is correct for this algorithm
	*flags |= PWVERIFY_FLAG_MYMODULE;

	const unsigned long long int passwdlen = (unsigned long long int) strlen(password);
	if (crypto_pwhash_scryptsalsa208sha256_str_verify(parameters, password, passwdlen) != 0)
	{
		(void) slog(LG_DEBUG, "%s: crypto_pwhash_scryptsalsa208sha256_str_verify() failed "
		                      "(incorrect password?)", MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}

static const struct crypt_impl crypto_sodium_scrypt_impl = {

	.id        = "sodium-scrypt",
	.verify    = &atheme_sodium_scrypt_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_sodium_scrypt_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_sodium_scrypt_impl);
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

SIMPLE_DECLARE_MODULE_V1("crypto/sodium-scrypt", MODULE_UNLOAD_CAPABILITY_OK)
