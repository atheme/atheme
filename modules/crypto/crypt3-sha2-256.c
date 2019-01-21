/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * POSIX-style SHA2-256 crypt(3) wrapper.
 */

#include "atheme.h"
#include "crypt3.h"

#ifdef HAVE_CRYPT

static bool ATHEME_FATTR_WUR
atheme_crypt3_sha2_256_selftest(void)
{
	static const char password[] = CRYPT3_MODULE_TEST_PASSWORD;
	static const char parameters[] = CRYPT3_MODULE_TEST_VECTOR_SHA2_256;

	const char *const result = atheme_crypt3_wrapper(password, parameters, MOWGLI_FUNC_NAME);

	if (! result)
		return false;

	if (strcmp(result, parameters) != 0)
	{
		(void) slog(LG_ERROR, "%s: crypt(3) returned an incorrect result", MOWGLI_FUNC_NAME);
		(void) slog(LG_ERROR, "%s: expected '%s', got '%s'", MOWGLI_FUNC_NAME, parameters, result);
		return false;
	}

	return true;
}

static const char * ATHEME_FATTR_WUR
atheme_crypt3_sha2_256_crypt(const char *const restrict password,
                             const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	static const char saltchars[CRYPT3_SHA2_SALTCHARS_LENGTH] = CRYPT3_SHA2_SALTCHARS;

	unsigned char rawsalt[CRYPT3_SHA2_SALTLENGTH];
	char salt[sizeof rawsalt + 1];
	char parv[PASSLEN + 1];

	(void) atheme_random_buf(rawsalt, sizeof rawsalt);
	(void) memset(salt, 0x00, sizeof salt);

	for (size_t i = 0; i < sizeof rawsalt; i++)
		salt[i] = saltchars[rawsalt[i] % sizeof saltchars];

	if (snprintf(parv, sizeof parv, CRYPT3_SAVESALT_FORMAT_SHA2_256, salt) > PASSLEN)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) output would have been too long (BUG)", MOWGLI_FUNC_NAME);
		return NULL;
	}

	(void) slog(LG_DEBUG, CRYPT3_MODULE_WARNING, MOWGLI_FUNC_NAME);

	return atheme_crypt3_wrapper(password, parv, MOWGLI_FUNC_NAME);
}

static bool ATHEME_FATTR_WUR
atheme_crypt3_sha2_256_verify(const char *const restrict password, const char *const restrict parameters,
                              unsigned int *const restrict flags)
{
	char hash[BUFSIZE];

	if (sscanf(parameters, CRYPT3_LOADHASH_FORMAT_SHA2_256, hash) != 1)
	{
		if (sscanf(parameters, CRYPT3_LOADHASH_FORMAT_SHA2_256_EXT, hash) != 1)
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

	*flags |= PWVERIFY_FLAG_MYMODULE;

	(void) slog(LG_DEBUG, CRYPT3_MODULE_WARNING, MOWGLI_FUNC_NAME);

	const char *const result = atheme_crypt3_wrapper(password, parameters, MOWGLI_FUNC_NAME);

	if (! result)
		return false;

	if (strcmp(parameters, result) != 0)
	{
		(void) slog(LG_DEBUG, "%s: strcmp(3) mismatch, invalid password?", MOWGLI_FUNC_NAME);
		(void) slog(LG_DEBUG, "%s: expected '%s', got '%s'", MOWGLI_FUNC_NAME, parameters, result);
		return false;
	}

	return true;
}

static const struct crypt_impl crypto_crypt3_impl = {

	.id        = "crypt3-sha2-256",
	.crypt     = &atheme_crypt3_sha2_256_crypt,
	.verify    = &atheme_crypt3_sha2_256_verify,
};

static void
mod_init(struct module *const restrict m)
{
	if (! atheme_crypt3_sha2_256_selftest())
	{
		(void) slog(LG_ERROR, "%s: self-test failed, does this platform support this algorithm?", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) slog(LG_INFO, CRYPT3_MODULE_WARNING, m->name);

	(void) crypt_register(&crypto_crypt3_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
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

SIMPLE_DECLARE_MODULE_V1("crypto/crypt3-sha2-256", MODULE_UNLOAD_CAPABILITY_OK)
