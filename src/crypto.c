/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Cryptographic module support.
 *
 * $Id: crypto.c 3551 2005-11-06 09:11:45Z alambert $
 */

#include "atheme.h"

static char saltbuf[BUFSIZE];
boolean_t crypto_module_loaded = FALSE;

/*
 * crypt_string is just like crypt(3) under UNIX
 * systems. Modules provide this function, otherwise
 * it returns the string unencrypted.
 */
char *(*crypt_string) (char *str, char *salt) = &generic_crypt_string;

char *generic_crypt_string(char *str, char *salt)
{
	return str;
}

/*
 * verify_password is a frontend to crypt_string().
 */
boolean_t verify_password(char *uinput, char *pass)
{
	char *cstr = crypt_string(uinput, pass);

	if (!strcmp(cstr, pass))
		return TRUE;

	return FALSE;
}

char *gen_salt(void)
{
	char *ht = gen_pw(6);

	strlcpy(saltbuf, "$1$", BUFSIZE);
	strlcat(saltbuf, ht, BUFSIZE);
	strlcat(saltbuf, "$", BUFSIZE);

	free(ht);

	return saltbuf;
}
