/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Cryptographic module support.
 *
 * $Id: crypto.c 1763 2005-08-18 18:45:28Z nenolod $
 */

#include "atheme.h"

/*
 * crypt_string is just like crypt(3) under UNIX
 * systems. Modules provide this function, otherwise
 * it returns the string unencrypted.
 */
char *(*crypt_string)(char *str, char *salt) = &generic_crypt_string;

char *generic_crypt_string(char *str, char *salt)
{
	return str;
}
