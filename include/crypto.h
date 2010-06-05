/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 *
 */

#ifndef CRYPTO_H
#define CRYPTO_H

E const char *(*crypt_string)(const char *str, const char *salt);
E const char *generic_crypt_string(const char *str, const char *salt);
E bool crypt_verify_password(const char *uinput, const char *pass);
E const char *gen_salt(void);
E bool crypto_module_loaded;

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
