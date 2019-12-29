/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Argon2 (Password Hashing Competition 2015 winner) password encryption
 * module, binding to the external reference libargon2 library built from
 * <https://github.com/P-H-C/phc-winner-argon2> available on many platforms.
 */

#include <atheme.h>

#ifdef HAVE_LIBARGON2

#include <argon2.h>

#define CRYPTO_MODULE_NAME      "crypto/argon2"

#define MODULE_SAVEHASH_FORMAT  "$%s$v=%u$m=%u,t=%u,p=%u$%s$%s"
#define MODULE_LOADHASH_FORMAT  "$%*[A-Za-z0-9]$v=%" SCNu32 "$m=%" SCNu32 ",t=%" SCNu32 ",p=%" SCNu32 "$" \
                                "%[" BASE64_ALPHABET_RFC4648 "]$%[" BASE64_ALPHABET_RFC4648 "]"

// The Argon2 reference implementation does not use padding characters
#define argon2_base64_encode(src, srclen, dst, dstlen) \
        base64_encode_table((src), (srclen), (dst), (dstlen), BASE64_ALPHABET_RFC4648_NOPAD)

static mowgli_list_t **crypto_conf_table = NULL;

static argon2_type atheme_argon2_type = Argon2_id;
static unsigned int atheme_argon2_memcost = ARGON2_MEMCOST_DEF;
static unsigned int atheme_argon2_timecost = ARGON2_TIMECOST_DEF;
static unsigned int atheme_argon2_threads = ARGON2_THREADS_DEF;
static unsigned int atheme_argon2_saltlen = ARGON2_SALTLEN_DEF;
static unsigned int atheme_argon2_hashlen = ARGON2_HASHLEN_DEF;

static int
c_ci_argon2_type(mowgli_config_file_entry_t *const restrict ce)
{
	atheme_argon2_type = Argon2_id;

	if (! ce->vardata)
	{
		(void) conf_report_warning(ce, "no parameter for configuration option -- using default");
		return 0;
	}

	if (strcasecmp(ce->vardata, "argon2d") == 0)
		atheme_argon2_type = Argon2_d;
	else if (strcasecmp(ce->vardata, "argon2i") == 0)
		atheme_argon2_type = Argon2_i;
	else if (strcasecmp(ce->vardata, "argon2id") == 0)
		atheme_argon2_type = Argon2_id;
	else
		(void) conf_report_warning(ce, "invalid parameter for configuration option -- using default");

	return 0;
}

static inline bool
atheme_argon2_needs_rehash(const uint32_t ver, const argon2_type type, const uint32_t m_cost, const uint32_t t_cost,
                           const uint32_t threads, const size_t saltlen, const size_t hashlen)
{
	if (ver != ARGON2_VERSION_NUMBER)
	{
		(void) slog(LG_DEBUG, "%s: ver (0x%" PRIu32 ") is not current (0x%04X)", MOWGLI_FUNC_NAME,
		                      ver, (unsigned int) ARGON2_VERSION_NUMBER);
		return true;
	}
	if (type != atheme_argon2_type)
	{
		(void) slog(LG_DEBUG, "%s: type (%s) is not the configured type (%s)", MOWGLI_FUNC_NAME,
		                      argon2_type2string(type, 0), argon2_type2string(atheme_argon2_type, 0));
		return true;
	}
	if (m_cost != (1U << atheme_argon2_memcost))
	{
		(void) slog(LG_DEBUG, "%s: m_cost (%" PRIu32 ") is not the configured value (%u)", MOWGLI_FUNC_NAME,
		                      m_cost, (1U << atheme_argon2_memcost));
		return true;
	}
	if (t_cost != atheme_argon2_timecost)
	{
		(void) slog(LG_DEBUG, "%s: t_cost (%" PRIu32 ") is not the configured value (%u)", MOWGLI_FUNC_NAME,
		                      t_cost, atheme_argon2_timecost);
		return true;
	}
	if (threads != atheme_argon2_threads)
	{
		(void) slog(LG_DEBUG, "%s: threads (%" PRIu32 ") is not the configured value (%u)", MOWGLI_FUNC_NAME,
		                      threads, atheme_argon2_threads);
		return true;
	}
	if (saltlen != atheme_argon2_saltlen)
	{
		(void) slog(LG_DEBUG, "%s: saltlen (%zu) is not the configured value (%u)", MOWGLI_FUNC_NAME,
		                      saltlen, atheme_argon2_saltlen);
		return true;
	}
	if (hashlen != atheme_argon2_hashlen)
	{
		(void) slog(LG_DEBUG, "%s: hashlen (%zu) is not the configured value (%u)", MOWGLI_FUNC_NAME,
		                      hashlen, atheme_argon2_hashlen);
		return true;
	}

	return false;
}

static bool
atheme_argon2_compute(argon2_context *const restrict ctx, const argon2_type type, const char *const restrict password)
{
	unsigned char pass[PASSLEN + 1];
	sigset_t oldset;
	sigset_t newset;

	/* The Argon2 library may spawn threads to handle this computation
	 * (even with ctx.threads == 1), and this code-base has not been
	 * audited for thread-safety, so block all signals before invoking
	 * it, and restore the set of blocked signals after.
	 *
	 * Newly-created threads (if any) inherit the blocked signal set of
	 * their parent, so signals delivered to this process during this
	 * computation will be deferred until the computation has finished,
	 * and then will only be handled by the main thread (this one).
	 *
	 *     -- amdj
	 */
	if (sigfillset(&newset) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigfillset(3): %s", MOWGLI_FUNC_NAME, strerror(errno));
		return false;
	}
	if (sigprocmask(SIG_BLOCK, &newset, &oldset) != 0)
	{
		(void) slog(LG_ERROR, "%s: sigprocmask(2): %s", MOWGLI_FUNC_NAME, strerror(errno));
		return false;
	}

	const uint32_t passlen = (uint32_t) strlen(password);
	bool result = false;
	int ret;

	(void) memcpy(pass, password, passlen);

	ctx->pwd = pass;
	ctx->pwdlen = passlen;
	ctx->lanes = ctx->threads;

	if ((ret = argon2_ctx(ctx, type)) != (int) ARGON2_OK)
		(void) slog(LG_ERROR, "%s: argon2_ctx() failed: %s", MOWGLI_FUNC_NAME, argon2_error_message(ret));
	else
		result = true;

	if (sigprocmask(SIG_SETMASK, &oldset, NULL) != 0)
		(void) slog(LG_ERROR, "%s: sigprocmask(2): %s", MOWGLI_FUNC_NAME, strerror(errno));

	(void) smemzero(pass, sizeof pass);
	return result;
}

static const char * ATHEME_FATTR_WUR
atheme_argon2_crypt(const char *const restrict password,
                    const char ATHEME_VATTR_UNUSED *const restrict parameters)
{
	static char resultbuf[PASSLEN + 1];
	unsigned char hash[ARGON2_HASHLEN_MAX];
	unsigned char salt[ARGON2_SALTLEN_MAX];
	char hash64[BASE64_SIZE_STR(sizeof hash)];
	char salt64[BASE64_SIZE_STR(sizeof salt)];
	const char *result = NULL;

	const char *const typestr = argon2_type2string(atheme_argon2_type, 0);

	(void) atheme_random_buf(salt, atheme_argon2_saltlen);

	argon2_context ctx = {
		.version    = ARGON2_VERSION_NUMBER,
		.m_cost     = (1U << atheme_argon2_memcost),
		.t_cost     = atheme_argon2_timecost,
		.threads    = atheme_argon2_threads,
		.salt       = salt,
		.saltlen    = (uint32_t) atheme_argon2_saltlen,
		.out        = hash,
		.outlen     = (uint32_t) atheme_argon2_hashlen,
	};

	if (! atheme_argon2_compute(&ctx, atheme_argon2_type, password))
		// This function logs messages on failure
		goto cleanup;

	if (argon2_base64_encode(salt, atheme_argon2_saltlen, salt64, sizeof salt64) == BASE64_FAIL)
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for salt failed (BUG)", MOWGLI_FUNC_NAME);
		goto cleanup;
	}
	if (argon2_base64_encode(hash, atheme_argon2_hashlen, hash64, sizeof hash64) == BASE64_FAIL)
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for hash failed (BUG)", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	if (snprintf(resultbuf, sizeof resultbuf, MODULE_SAVEHASH_FORMAT, argon2_type2string(atheme_argon2_type, 0),
	               ctx.version, ctx.m_cost, ctx.t_cost, atheme_argon2_threads, salt64, hash64) >= (int) PASSLEN)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", MOWGLI_FUNC_NAME);
		(void) smemzero(resultbuf, sizeof resultbuf);
		goto cleanup;
	}

	result = resultbuf;

cleanup:
	(void) smemzero(hash64, sizeof hash64);
	(void) smemzero(hash, sizeof hash);
	return result;
}

static bool ATHEME_FATTR_WUR
atheme_argon2_verify(const char *const restrict password, const char *const restrict parameters,
                     unsigned int *const restrict flags)
{
	bool result = false;

	argon2_type type;
	uint32_t ver;
	uint32_t m_cost;
	uint32_t t_cost;
	uint32_t threads;
	unsigned char salt[BUFSIZE];
	unsigned char hash[BUFSIZE];
	unsigned char cmphash[sizeof hash];
	char salt64[BASE64_SIZE_STR(sizeof salt)];
	char hash64[BASE64_SIZE_STR(sizeof hash)];
	size_t saltlen;
	size_t hashlen;

	if (strncasecmp(parameters, "$argon2d$", 9U) == 0)
		type = Argon2_d;
	else if (strncasecmp(parameters, "$argon2i$", 9U) == 0)
		type = Argon2_i;
	else if (strncasecmp(parameters, "$argon2id$", 10U) == 0)
		type = Argon2_id;
	else
	{
		(void) slog(LG_DEBUG, "%s: prefix does not match", MOWGLI_FUNC_NAME);
		return false;
	}

	if (sscanf(parameters, MODULE_LOADHASH_FORMAT, &ver, &m_cost, &t_cost, &threads, salt64, hash64) != 6)
	{
		(void) slog(LG_DEBUG, "%s: sscanf(3) was unsuccessful", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	*flags |= PWVERIFY_FLAG_MYMODULE;

	if ((saltlen = base64_decode(salt64, salt, sizeof salt)) < ARGON2_SALTLEN_MIN)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode() for salt failed", MOWGLI_FUNC_NAME);
		goto cleanup;
	}
	if ((hashlen = base64_decode(hash64, hash, sizeof hash)) < ARGON2_HASHLEN_MIN)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode() for hash failed", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	argon2_context ctx = {
		.version        = ver,
		.m_cost         = m_cost,
		.t_cost         = t_cost,
		.threads        = threads,
		.salt           = salt,
		.saltlen        = (uint32_t) saltlen,
		.out            = cmphash,
		.outlen         = (uint32_t) hashlen,
	};

	if (! atheme_argon2_compute(&ctx, type, password))
		// This function logs messages on failure
		goto cleanup;

	if (smemcmp(hash, cmphash, hashlen) != 0)
		goto cleanup;

	(void) slog(LG_DEBUG, "%s: authentication successful", MOWGLI_FUNC_NAME);

	if (atheme_argon2_needs_rehash(ver, type, m_cost, t_cost, threads, saltlen, hashlen))
		*flags |= PWVERIFY_FLAG_RECRYPT;

	result = true;

cleanup:
	(void) smemzero(cmphash, sizeof cmphash);
	(void) smemzero(hash64, sizeof hash64);
	(void) smemzero(hash, sizeof hash);
	return result;
}

static const struct crypt_impl crypto_argon2_impl = {

	.id         = CRYPTO_MODULE_NAME,
	.crypt      = &atheme_argon2_crypt,
	.verify     = &atheme_argon2_verify,
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, crypto_conf_table, "crypto/main", "crypto_conf_table")

	(void) add_conf_item("argon2_type", *crypto_conf_table, &c_ci_argon2_type);

	(void) add_uint_conf_item("argon2_memcost", *crypto_conf_table, 0, &atheme_argon2_memcost,
	                          ARGON2_MEMCOST_MIN, ARGON2_MEMCOST_MAX, ARGON2_MEMCOST_DEF);

	(void) add_uint_conf_item("argon2_timecost", *crypto_conf_table, 0, &atheme_argon2_timecost,
	                          ARGON2_TIMECOST_MIN, ARGON2_TIMECOST_MAX, ARGON2_TIMECOST_DEF);

	(void) add_uint_conf_item("argon2_threads", *crypto_conf_table, 0, &atheme_argon2_threads,
	                          ARGON2_THREADS_MIN, ARGON2_THREADS_MAX, ARGON2_THREADS_DEF);

	(void) add_uint_conf_item("argon2_saltlen", *crypto_conf_table, 0, &atheme_argon2_saltlen,
	                          ARGON2_SALTLEN_MIN, ARGON2_SALTLEN_MAX, ARGON2_SALTLEN_DEF);

	(void) add_uint_conf_item("argon2_hashlen", *crypto_conf_table, 0, &atheme_argon2_hashlen,
	                          ARGON2_HASHLEN_MIN, ARGON2_HASHLEN_MAX, ARGON2_HASHLEN_DEF);

	(void) crypt_register(&crypto_argon2_impl);

	m->mflags |= MODFLAG_DBCRYPTO;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) crypt_unregister(&crypto_argon2_impl);

	(void) del_conf_item("argon2_type", *crypto_conf_table);
	(void) del_conf_item("argon2_memcost", *crypto_conf_table);
	(void) del_conf_item("argon2_timecost", *crypto_conf_table);
	(void) del_conf_item("argon2_threads", *crypto_conf_table);
	(void) del_conf_item("argon2_saltlen", *crypto_conf_table);
	(void) del_conf_item("argon2_hashlen", *crypto_conf_table);
}

#else /* HAVE_LIBARGON2 */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "%s: this module requires libargon2; refusing to load", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Do Nothing
}

#endif /* !HAVE_LIBARGON2 */

SIMPLE_DECLARE_MODULE_V1(CRYPTO_MODULE_NAME, MODULE_UNLOAD_CAPABILITY_OK)
