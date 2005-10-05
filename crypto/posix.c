/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * POSIX-style crypt(3) wrapper.
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/posix", FALSE, _modinit, _moddeinit,
	"$Id$",
	"Atheme Development Group <http://www.atheme.org>"
);

static char *posix_crypt_string(char *key, char *salt)
{
	return crypt(key, salt);
}

void _modinit(module_t *m)
{
	crypt_string = &posix_crypt_string;
}

void _moddeinit(void)
{
	crypt_string = &generic_crypt_string;
}
