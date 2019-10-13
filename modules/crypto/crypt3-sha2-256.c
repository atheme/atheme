/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * POSIX-style SHA2-256 crypt(3) wrapper.
 */

#include <atheme.h>

#define CRYPTO_MODULE_NAME "crypto/crypt3-sha2-256"

#ifdef HAVE_CRYPT

#include "crypt3-wrapper.h"

static mowgli_list_t **crypto_conf_table = NULL;

static unsigned int crypt3_md_rounds = CRYPT3_SHA2_ITERCNT_DEF;

static const char * ATHEME_FATTR_WUR
atheme_crypt3_sha2_256_crypt(const char *const restrict password,
                             const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	unsigned char rawsalt[CRYPT3_SHA2_SALTLEN_RAW];
	char salt[BASE64_SIZE_STR(sizeof rawsalt)];
	char parv[PASSLEN + 1];

	(void) atheme_random_buf(rawsalt, sizeof rawsalt);

	if (base64_encode_table(rawsalt, sizeof rawsalt, salt, sizeof salt, BASE64_ALPHABET_CRYPT3) == BASE64_FAIL)
	{
		(void) slog(LG_ERROR, "%s: base64_encode_table() failed (BUG!)", MOWGLI_FUNC_NAME);
		return NULL;
	}

	if (crypt3_md_rounds == CRYPT3_SHA2_ITERCNT_DEF)
	{
		if (snprintf(parv, sizeof parv, CRYPT3_SAVESALT_FORMAT_SHA2_256, salt) > PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) output would have been too long (BUG)", MOWGLI_FUNC_NAME);
			return NULL;
		}
	}
	else
	{
		if (snprintf(parv, sizeof parv, CRYPT3_SAVESALT_FORMAT_SHA2_256_EXT, crypt3_md_rounds, salt) > PASSLEN)
		{
			(void) slog(LG_ERROR, "%s: snprintf(3) output would have been too long (BUG)", MOWGLI_FUNC_NAME);
			return NULL;
		}
	}

	// This function logs messages on failure
	return atheme_crypt3_wrapper(password, parv, MOWGLI_FUNC_NAME);
}

static bool ATHEME_FATTR_WUR
atheme_crypt3_sha2_256_verify(const char *const restrict password, const char *const restrict parameters,
                              unsigned int *const restrict flags)
{
	unsigned int rounds = CRYPT3_SHA2_ITERCNT_DEF;
	char hash[BUFSIZE];

	if (sscanf(parameters, CRYPT3_LOADHASH_FORMAT_SHA2_256, hash) != 1)
	{
		if (sscanf(parameters, CRYPT3_LOADHASH_FORMAT_SHA2_256_EXT, &rounds, hash) != 2)
		{
			(void) slog(LG_DEBUG, "%s: sscanf(3) was unsuccessful", MOWGLI_FUNC_NAME);
			return false;
		}
	}
	if (strlen(hash) != CRYPT3_LOADHASH_LENGTH_SHA2_256)
	{
		(void) slog(LG_DEBUG, "%s: digest not %u characters long", MOWGLI_FUNC_NAME,
		                      CRYPT3_LOADHASH_LENGTH_SHA2_256);
		return false;
	}

	(void) smemzero(hash, sizeof hash);

	*flags |= PWVERIFY_FLAG_MYMODULE;

	const char *const result = atheme_crypt3_wrapper(password, parameters, MOWGLI_FUNC_NAME);

	if (! result)
		// That function logs messages on failure
		return false;

	if (strcmp(parameters, result) != 0)
	{
		(void) slog(LG_DEBUG, "%s: strcmp(3) mismatch, invalid password?", MOWGLI_FUNC_NAME);
		(void) slog(LG_DEBUG, "%s: expected '%s', got '%s'", MOWGLI_FUNC_NAME, parameters, result);
		return false;
	}

	if (rounds != crypt3_md_rounds)
	{
		(void) slog(LG_DEBUG, "%s: rounds (%u) != default (%u)", MOWGLI_FUNC_NAME, rounds, crypt3_md_rounds);

		*flags |= PWVERIFY_FLAG_RECRYPT;
	}

	return true;
}

static const struct crypt_impl crypto_crypt3_impl = {

	.id        = CRYPTO_MODULE_NAME,
	.crypt     = &atheme_crypt3_sha2_256_crypt,
	.verify    = &atheme_crypt3_sha2_256_verify,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table")

	if (! atheme_crypt3_selftest(true, CRYPT3_MODULE_TEST_VECTOR_SHA2_256))
	{
		(void) slog(LG_ERROR, "%s: self-test failed, does this platform support this algorithm?", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (atheme_crypt3_selftest(false, CRYPT3_MODULE_TEST_VECTOR_SHA2_256_EXT))
		(void) add_uint_conf_item("crypt3_sha2_256_rounds", *crypto_conf_table, 0, &crypt3_md_rounds,
		                          CRYPT3_SHA2_ITERCNT_MIN, CRYPT3_SHA2_ITERCNT_MAX, CRYPT3_SHA2_ITERCNT_DEF);
	else
		(void) slog(LG_INFO, "%s: the number of rounds is not configurable on this platform", m->name);

	(void) slog(LG_INFO, CRYPT3_MODULE_WARNING, m->name);

	(void) crypt_register(&crypto_crypt3_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("crypt3_sha2_256_rounds", *crypto_conf_table);

	(void) crypt_unregister(&crypto_crypt3_impl);
}

#else /* HAVE_CRYPT */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires crypt(3) support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_CRYPT */

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
