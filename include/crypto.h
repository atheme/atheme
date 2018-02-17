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

typedef struct {

	const char *id;
	const char *(*crypt)(const char *password, const char *parameters);
	bool (*verify)(const char *password, const char *parameters, unsigned int *flags);

	mowgli_node_t node;

} crypt_impl_t;

extern void crypt_register(crypt_impl_t *impl);
extern void crypt_unregister(crypt_impl_t *impl);

extern const crypt_impl_t *crypt_get_default_provider(void);
extern const crypt_impl_t *crypt_verify_password(const char *password, const char *parameters, unsigned int *flags);
extern const char *crypt_password(const char *password);

#endif
