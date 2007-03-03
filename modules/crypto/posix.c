/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * POSIX-style crypt(3) wrapper.
 *
 * $Id: posix.c 7771 2007-03-03 12:46:36Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/posix", FALSE, _modinit, _moddeinit,
	"$Id: posix.c 7771 2007-03-03 12:46:36Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static char *posix_crypt_string(char *key, char *salt)
{
	return crypt(key, salt);
}

void _modinit(module_t *m)
{
	crypt_string = &posix_crypt_string;

	crypto_module_loaded = TRUE;
}

void _moddeinit(void)
{
	crypt_string = &generic_crypt_string;

	crypto_module_loaded = FALSE;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:noexpandtab
 */
