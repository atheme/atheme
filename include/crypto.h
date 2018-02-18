/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#define PWVERIFY_FLAG_NONE      0x0000U // Initial state
#define PWVERIFY_FLAG_MYMODULE  0x0001U // This password hash was from 'this' crypto module
#define PWVERIFY_FLAG_RECRYPT   0x0002U // This password needs re-encrypting

struct crypt_impl
{
	const char *id;
	const char *(*crypt)(const char *password, const char *parameters);
	bool (*verify)(const char *password, const char *parameters, unsigned int *flags);

	mowgli_node_t node;
};

extern void crypt_register(struct crypt_impl *impl);
extern void crypt_unregister(struct crypt_impl *impl);

extern const struct crypt_impl *crypt_get_default_provider(void);
extern const struct crypt_impl *crypt_verify_password(const char *password, const char *parameters, unsigned int *flags);
extern const char *crypt_password(const char *password);

#endif
