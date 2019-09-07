/*
 * SPDX-License-Identifier: GPL-2.0
 * SPDX-URL: https://spdx.org/licenses/GPL-2.0.html
 *
 * Copyright (C) 2003 Anope Team (https://www.anope.org/)
 * Copyright (C) 2005-2012 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * IRCServices' weird password encryption module, taken from Anope 1.6.3.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include <atheme.h>

#define CRYPTO_MODULE_NAME      "crypto/ircservices"

#define MODULE_PREFIX_STR       "$ircservices$"
#define MODULE_PREFIX_LEN       13
#define MODULE_DIGEST_LEN       8
#define MODULE_PARAMS_LEN       (MODULE_PREFIX_LEN + (2 * MODULE_DIGEST_LEN))

#define XTOI(c) ((c)>9 ? (c)-'A'+10 : (c)-'0')

static inline bool ATHEME_FATTR_WUR
atheme_ircservices_encrypt(const char *const restrict src, char *const restrict dest)
{
	char digest[33];
	(void) memset(digest, 0x00, sizeof digest);

	if (! digest_oneshot(DIGALG_MD5, src, strlen(src), digest, NULL))
		return false;

	char dest2[16];
	for (size_t i = 0; i < 32; i += 2)
		dest2[i / 2] = XTOI(digest[i]) << 4 | XTOI(digest[i + 1]);

	(void) strcpy(dest, MODULE_PREFIX_STR);
	for (size_t i = 0; i < MODULE_DIGEST_LEN; i++)
		(void) sprintf(dest + MODULE_PREFIX_LEN + 2 * i, "%02x", 255 & dest2[i]);

	return true;
}

static bool ATHEME_FATTR_WUR
atheme_ircservices_verify(const char *const restrict password, const char *const restrict parameters,
                          unsigned int *const restrict flags)
{
	if (strlen(parameters) != MODULE_PARAMS_LEN)
		return false;

	if (strncmp(parameters, MODULE_PREFIX_STR, MODULE_PREFIX_LEN) != 0)
		return false;

	*flags |= PWVERIFY_FLAG_MYMODULE;

	char result[BUFSIZE];

	if (! atheme_ircservices_encrypt(password, result))
		return false;

	if (strcmp(result, parameters) != 0)
		return false;

	(void) smemzero(result, sizeof result);

	return true;
}

static const struct crypt_impl crypto_ircservices_impl = {

	.id         = CRYPTO_MODULE_NAME,
	.verify     = &atheme_ircservices_verify,
};

static void
mod_init(struct module *const restrict m)
{
	(void) crypt_register(&crypto_ircservices_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_ircservices_impl);
}

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
