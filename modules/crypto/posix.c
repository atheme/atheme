/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * POSIX-style crypt(3) wrapper.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"crypto/posix", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static const char *posixc_crypt_string(const char *key, const char *salt)
{
	const char *result;
	char salt2[3];
	static int warned;

	result = crypt(key, salt);
	if (!strncmp(salt, "$1$", 3) && strncmp(result, "$1$", 3))
	{
		if (!warned)
			slog(LG_INFO, "posixc_crypt_string(): broken crypt() detected, falling back to DES");
		warned = 1;
		salt2[0] = salt[3];
		salt2[1] = salt[4];
		salt2[2] = '\0';
		result = crypt(key, salt2);
	}
	return result;
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
