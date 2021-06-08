/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * ARM mbedTLS frontend for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

#ifdef MBEDTLS_CONFIG_FILE
#  include MBEDTLS_CONFIG_FILE
#else
#  include <mbedtls/config.h>
#endif

#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/version.h>

const char *
digest_get_frontend_info(void)
{
	char verbuf[64];
	(void) memset(verbuf, 0x00, sizeof verbuf);
	(void) mbedtls_version_get_string(verbuf);

	static char result[BUFSIZE];
	(void) snprintf(result, sizeof result, "ARM mbedTLS (compiled %s, library %s)", MBEDTLS_VERSION_STRING, verbuf);

	return result;
}

static inline const mbedtls_md_info_t *
_digest_decide_md(const enum digest_algorithm alg)
{
	mbedtls_md_type_t md_type = MBEDTLS_MD_NONE;

	switch (alg)
	{
		case DIGALG_MD5:
			md_type = MBEDTLS_MD_MD5;
			break;

		case DIGALG_SHA1:
			md_type = MBEDTLS_MD_SHA1;
			break;

		case DIGALG_SHA2_256:
			md_type = MBEDTLS_MD_SHA256;
			break;

		case DIGALG_SHA2_512:
			md_type = MBEDTLS_MD_SHA512;
			break;
	}

	if (md_type == MBEDTLS_MD_NONE)
	{
		(void) slog(LG_ERROR, "%s: unrecognised algorithm '%u' (BUG)", MOWGLI_FUNC_NAME, (unsigned int) alg);
		return NULL;
	}

	const mbedtls_md_info_t *const md_info = mbedtls_md_info_from_type(md_type);

	if (! md_info)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_md_info_from_type() failed (BUG?)", MOWGLI_FUNC_NAME);
		return NULL;
	}

	return md_info;
}

static bool ATHEME_FATTR_WUR
_digest_init(struct digest_context *const restrict ctx, const enum digest_algorithm alg)
{
	(void) memset(ctx, 0x00, sizeof *ctx);

	ctx->alg = alg;

	if (! (ctx->md = _digest_decide_md(alg)))
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}

	(void) mbedtls_md_init(&ctx->state);

	if (mbedtls_md_setup(&ctx->state, ctx->md, 0) != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_md_setup() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}
	if (mbedtls_md_starts(&ctx->state) != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_md_starts() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_init_hmac(struct digest_context *const restrict ctx, const enum digest_algorithm alg,
                  const void *const restrict key, const size_t keyLen)
{
	(void) memset(ctx, 0x00, sizeof *ctx);

	ctx->alg = alg;
	ctx->hmac = true;

	if (! (ctx->md = _digest_decide_md(alg)))
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}

	(void) mbedtls_md_init(&ctx->state);

	if (mbedtls_md_setup(&ctx->state, ctx->md, 1) != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_md_setup() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}
	if (mbedtls_md_hmac_starts(&ctx->state, key, keyLen) != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_md_hmac_starts() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (! (data && dataLen))
		return true;

	if (ctx->hmac)
	{
		if (mbedtls_md_hmac_update(&ctx->state, data, dataLen) != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_md_hmac_update() failed (BUG?)", MOWGLI_FUNC_NAME);
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}
	else
	{
		if (mbedtls_md_update(&ctx->state, data, dataLen) != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_md_update() failed (BUG?)", MOWGLI_FUNC_NAME);
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	const size_t hLen = digest_size_ctx(ctx);

	if (outLen)
		*outLen = hLen;

	if (ctx->hmac)
	{
		if (mbedtls_md_hmac_finish(&ctx->state, out) != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_md_hmac_finish() failed (BUG?)", MOWGLI_FUNC_NAME);
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}
	else
	{
		if (mbedtls_md_finish(&ctx->state, out) != 0)
		{
			(void) slog(LG_ERROR, "%s: mbedtls_md_finish() failed (BUG?)", MOWGLI_FUNC_NAME);
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}

	(void) mbedtls_md_free(&ctx->state);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_pbkdf2(const enum digest_algorithm alg, const void *const restrict pass, const size_t passLen,
                       const void *const restrict salt, const size_t saltLen, const size_t c,
                       void *const restrict dk, const size_t dkLen)
{
	mbedtls_md_context_t ctx;

	(void) mbedtls_md_init(&ctx);

	const mbedtls_md_info_t *const md = _digest_decide_md(alg);

	if (! md)
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx);
		return false;
	}
	if (mbedtls_md_setup(&ctx, md, 1) != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_md_setup() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx);
		return false;
	}
	if (mbedtls_pkcs5_pbkdf2_hmac(&ctx, pass, passLen, salt, saltLen, (unsigned int) c, (uint32_t) dkLen, dk) != 0)
	{
		(void) slog(LG_ERROR, "%s: mbedtls_pkcs5_pbkdf2_hmac() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) mbedtls_md_free(&ctx);
		return false;
	}

	(void) mbedtls_md_free(&ctx);
	return true;
}
