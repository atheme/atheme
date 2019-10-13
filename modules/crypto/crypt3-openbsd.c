/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * OpenBSD-style crypt_checkpass(3)/crypt_newhash(3) wrapper.
 */

#include <atheme.h>

#define CRYPTO_MODULE_NAME "crypto/crypt3-openbsd"

#if defined(__OpenBSD__) && defined(HAVE_CRYPT_CHECKPASS) && defined(HAVE_CRYPT_NEWHASH)

#define CRYPT3_PREF_DEF "bcrypt,a"

static char *crypt3_pref = NULL;

static mowgli_list_t **crypto_conf_table = NULL;

static const char * ATHEME_FATTR_WUR
atheme_crypt3_openbsd_crypt(const char *const restrict password,
                            const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	static char result[PASSLEN + 1];

	if (crypt_newhash(password, crypt3_pref, result, sizeof result) != 0)
	{
		(void) slog(LG_ERROR, "%s: crypt_newhash(3): %s", MOWGLI_FUNC_NAME, strerror(errno));
		return NULL;
	}

	return result;
}

static bool ATHEME_FATTR_WUR
atheme_crypt3_openbsd_verify(const char *const restrict password, const char *const restrict parameters,
                             unsigned int ATHEME_VATTR_UNUSED *const restrict flags)
{
	if (crypt_checkpass(password, parameters) != 0)
	{
		(void) slog(LG_DEBUG, "%s: crypt_checkpass(3): %s", MOWGLI_FUNC_NAME, strerror(errno));
		return false;
	}

	return true;
}

static const struct crypt_impl crypto_crypt3_impl = {

	.id        = CRYPTO_MODULE_NAME,
	.crypt     = &atheme_crypt3_openbsd_crypt,
	.verify    = &atheme_crypt3_openbsd_verify,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table")

	(void) add_dupstr_conf_item("crypt3_openbsd_newhash_pref", *crypto_conf_table, 0, &crypt3_pref, CRYPT3_PREF_DEF);

	(void) crypt_register(&crypto_crypt3_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("crypt3_openbsd_newhash_pref", *crypto_conf_table);

	(void) crypt_unregister(&crypto_crypt3_impl);
}

#else /* __OpenBSD__ && HAVE_CRYPT_CHECKPASS && HAVE_CRYPT_NEWHASH */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires a recent OpenBSD system; refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !(__OpenBSD__ && HAVE_CRYPT_CHECKPASS && HAVE_CRYPT_NEWHASH) */

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
