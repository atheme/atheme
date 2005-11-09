/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * POSIX-style crypt(3) wrapper.
 *
 * $Id: posix.c 3685 2005-11-09 01:07:04Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/posix", FALSE, _modinit, _moddeinit,
	"$Id: posix.c 3685 2005-11-09 01:07:04Z alambert $",
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
