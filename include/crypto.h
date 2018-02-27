/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 */

#ifndef ATHEME_INC_CRYPTO_H
#define ATHEME_INC_CRYPTO_H 1

#define PWVERIFY_FLAG_NONE      0x0000U // Initial state
#define PWVERIFY_FLAG_MYMODULE  0x0001U // This password hash was from 'this' crypto module
#define PWVERIFY_FLAG_RECRYPT   0x0002U // This password needs re-encrypting

struct crypt_impl
{
	const char *    id;
	const char *  (*crypt)(const char *password, const char *parameters);
	bool          (*verify)(const char *password, const char *parameters, unsigned int *flags);
};

void crypt_register(const struct crypt_impl *impl);
void crypt_unregister(const struct crypt_impl *impl);

const struct crypt_impl *crypt_get_default_provider(void);
const struct crypt_impl *crypt_verify_password(const char *password, const char *parameters, unsigned int *flags);
const char *crypt_password(const char *password);

#endif /* !ATHEME_INC_CRYPTO_H */
