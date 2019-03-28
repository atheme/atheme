/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Cryptographic module support.
 */

#ifndef ATHEME_INC_CRYPTO_H
#define ATHEME_INC_CRYPTO_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

#define PWVERIFY_FLAG_NONE      0x0000U // Initial state
#define PWVERIFY_FLAG_MYMODULE  0x0001U // This password hash was from 'this' crypto module
#define PWVERIFY_FLAG_RECRYPT   0x0002U // This password needs re-encrypting

typedef const char *(*crypt_crypt_func)(const char *, const char *) ATHEME_FATTR_WUR;
typedef bool (*crypt_verify_func)(const char *, const char *, unsigned int *) ATHEME_FATTR_WUR;

struct crypt_impl
{
	const char *            id;
	crypt_crypt_func        crypt;
	crypt_verify_func       verify;
};

void crypt_register(const struct crypt_impl *impl);
void crypt_unregister(const struct crypt_impl *impl);

const struct crypt_impl *crypt_get_default_provider(void);
const struct crypt_impl *crypt_get_named_provider(const char *id);
const struct crypt_impl *crypt_verify_password(const char *password, const char *parameters, unsigned int *flags)
    ATHEME_FATTR_WUR;

const char *crypt_password(const char *password);

#endif /* !ATHEME_INC_CRYPTO_H */
