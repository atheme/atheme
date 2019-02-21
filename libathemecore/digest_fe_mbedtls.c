/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * ARM mbedTLS frontend for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/version.h>

static inline const mbedtls_md_info_t *
digest_decide_md(const enum digest_algorithm alg)
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

	return mbedtls_md_info_from_type(md_type);
}

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

bool ATHEME_FATTR_WUR
digest_init(struct digest_context *const restrict ctx, const enum digest_algorithm alg)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	(void) memset(ctx, 0x00, sizeof *ctx);

	if (! (ctx->md = digest_decide_md(alg)))
		return false;

	(void) mbedtls_md_init(&ctx->state);

	if (mbedtls_md_setup(&ctx->state, ctx->md, 0) != 0)
	{
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}
	if (mbedtls_md_starts(&ctx->state) != 0)
	{
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}

	return true;
}

bool ATHEME_FATTR_WUR
digest_init_hmac(struct digest_context *const restrict ctx, const enum digest_algorithm alg,
                 const void *const restrict key, const size_t keyLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	(void) memset(ctx, 0x00, sizeof *ctx);

	if (! (ctx->md = digest_decide_md(alg)))
		return false;

	ctx->hmac = true;

	(void) mbedtls_md_init(&ctx->state);

	if (mbedtls_md_setup(&ctx->state, ctx->md, 1) != 0)
	{
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}
	if (mbedtls_md_hmac_starts(&ctx->state, key, keyLen) != 0)
	{
		(void) mbedtls_md_free(&ctx->state);
		return false;
	}

	return true;
}

bool ATHEME_FATTR_WUR
digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	if (! (data && dataLen))
		return true;

	if (ctx->hmac)
	{
		if (mbedtls_md_hmac_update(&ctx->state, data, dataLen) != 0)
		{
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}
	else
	{
		if (mbedtls_md_update(&ctx->state, data, dataLen) != 0)
		{
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}

	return true;
}

bool ATHEME_FATTR_WUR
digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	const size_t hLen = (size_t) mbedtls_md_get_size(ctx->md);

	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	else if (outLen)
		*outLen = hLen;

	if (ctx->hmac)
	{
		if (mbedtls_md_hmac_finish(&ctx->state, out) != 0)
		{
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}
	else
	{
		if (mbedtls_md_finish(&ctx->state, out) != 0)
		{
			(void) mbedtls_md_free(&ctx->state);
			return false;
		}
	}

	(void) mbedtls_md_free(&ctx->state);
	return true;
}

bool ATHEME_FATTR_WUR
digest_pbkdf2_hmac(const enum digest_algorithm alg, const void *const restrict pass, const size_t passLen,
                   const void *const restrict salt, const size_t saltLen, const size_t c,
                   void *const restrict dk, const size_t dkLen)
{
	mbedtls_md_context_t ctx;

	(void) mbedtls_md_init(&ctx);

	if (! c)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'c' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! dk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'dk' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! dkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'dkLen' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if ((! pass && passLen) || (pass && ! passLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched pass parameters (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if ((! salt && saltLen) || (salt && ! saltLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched salt parameters (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}

	const mbedtls_md_info_t *const md = digest_decide_md(alg);

	if (! md)
		goto error;

	if (mbedtls_md_setup(&ctx, md, 1) != 0)
		goto error;

	if (mbedtls_pkcs5_pbkdf2_hmac(&ctx, pass, passLen, salt, saltLen, (unsigned int) c, (uint32_t) dkLen, dk) != 0)
		goto error;

	(void) mbedtls_md_free(&ctx);
	(void) smemzero(&ctx, sizeof ctx);
	return true;

error:
	(void) mbedtls_md_free(&ctx);
	(void) smemzero(&ctx, sizeof ctx);
	(void) smemzero(dk, dkLen);
	return false;
}
