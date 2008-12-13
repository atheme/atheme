/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * POSIX-style crypt(3) wrapper.
 *
 * $Id: posix.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/posix", false, _modinit, _moddeinit,
	"$Id: posix.c 7779 2007-03-03 13:55:42Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static const char *posixc_crypt_string(const char *key, const char *salt)
{
	return crypt(key, salt);
}

void _modinit(module_t *m)
{
	crypt_string = &posixc_crypt_string;

	crypto_module_loaded = true;
}

void _moddeinit(void)
{
	crypt_string = &generic_crypt_string;

	crypto_module_loaded = false;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
