/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 *
 */

#ifndef CRYPTO_H
#define CRYPTO_H

typedef struct {

	const char *id;
	const char *(*salt)(void);
	const char *(*crypt)(const char *password, const char *parameters);
	bool (*verify)(const char *password, const char *parameters);
	bool (*recrypt)(const char *parameters);

	mowgli_node_t node;
} crypt_impl_t;

E const crypt_impl_t *crypt_get_default_provider(void);
E void crypt_register(crypt_impl_t *impl);
E void crypt_unregister(crypt_impl_t *impl);

E const char *gen_salt(void);
E const char *crypt_string(const char *password, const char *parameters);

E const crypt_impl_t *crypt_verify_password(const char *password, const char *parameters);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
