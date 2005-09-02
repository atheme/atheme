/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 *
 * $Id: crypto.h 1763 2005-08-18 18:45:28Z nenolod $
 */

#ifndef CRYPTO_H
#define CRYPTO_H

extern char *(*crypt_string)(char *str, char *salt);
extern char *generic_crypt_string(char *str, char *salt);

#endif
