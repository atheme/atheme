/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * IRCServices's weird password encryption thingy, taken from Anope 1.6.3.
 */

/*
 * (C) 2003 Anope Team
 * Contact us at info@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "atheme.h"

#define XTOI(c) ((c)>9 ? (c)-'A'+10 : (c)-'0')

static inline void
atheme_ircservices_encrypt(const char *const restrict src, char *const restrict dest)
{
	char digest[33];
	(void) memset(digest, 0x00, sizeof digest);

	md5_state_t ctx;
	(void) md5_init(&ctx);
	(void) md5_append(&ctx, (const unsigned char *) src, strlen(src));
	(void) md5_finish(&ctx, (unsigned char *) digest);

	char dest2[16];
	for (size_t i = 0; i < 32; i += 2)
		dest2[i / 2] = XTOI(digest[i]) << 4 | XTOI(digest[i + 1]);

	(void) strcpy(dest, "$ircservices$");
	for (size_t i = 0; i < 8; i++)
		(void) sprintf(dest + 13 + 2 * i, "%02x", 255 & dest2[i]);
}

static bool
atheme_ircservices_verify(const char *const restrict password, const char *const restrict parameters)
{
	char result[BUFSIZE];

	(void) atheme_ircservices_encrypt(password, result);

	if (strcmp(result, parameters) != 0)
		return false;

	return true;
}

static crypt_impl_t crypto_ircservices_impl = {

	.id         = "ircservices",
	.verify     = &atheme_ircservices_verify,
};

static void
crypto_ircservices_modinit(module_t __attribute__((unused)) *const restrict m)
{
	(void) crypt_register(&crypto_ircservices_impl);
}

static void
crypto_ircservices_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) crypt_unregister(&crypto_ircservices_impl);
}

DECLARE_MODULE_V1("crypto/ircservices", false, crypto_ircservices_modinit, crypto_ircservices_moddeinit,
                  PACKAGE_STRING, "Jilles Tjoelker <jilles@stack.nl>");
