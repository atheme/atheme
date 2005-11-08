/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 *
 * $Id: crypto.h 3665 2005-11-08 03:09:32Z alambert $
 */

#ifndef CRYPTO_H
#define CRYPTO_H

E char *(*crypt_string)(char *str, char *salt);
E char *generic_crypt_string(char *str, char *salt);
E boolean_t crypt_verify_password(char *uinput, char *pass);
E char *gen_salt(void);
E boolean_t crypto_module_loaded;

#endif
