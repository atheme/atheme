/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 *
 */

#ifndef CRYPTO_H
#define CRYPTO_H

E const char *crypt_string(const char *key, const char *salt);
E const char *gen_salt(void);
E bool crypto_module_loaded;

typedef struct {
	const char *id;
	const char *(*crypt)(const char *key, const char *salt);
	const char *(*salt)(void);

	mowgli_node_t node;
} crypt_impl_t;

E void crypt_register(crypt_impl_t *impl);
E void crypt_unregister(crypt_impl_t *impl);
E const crypt_impl_t *crypt_verify_password(const char *user_input, const char *pass);
E const crypt_impl_t *crypt_get_default_provider(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
