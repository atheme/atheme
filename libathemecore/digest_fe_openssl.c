/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * OpenSSL frontend for the digest interface.
 */

#include "atheme.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>

static inline const EVP_MD *
digest_decide_md(const unsigned int alg)
{
	switch (alg)
	{
		case DIGALG_MD5:
			return EVP_md5();

		case DIGALG_SHA1:
			return EVP_sha1();

		case DIGALG_SHA2_256:
			return EVP_sha256();

		case DIGALG_SHA2_512:
			return EVP_sha512();
	}

	(void) slog(LG_ERROR, "%s: called with unknown/unimplemented alg '%u' (BUG)", __func__, alg);
	return NULL;
}

bool ATHEME_FATTR_WUR
digest_init(struct digest_context *const restrict ctx, const unsigned int alg)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	(void) memset(ctx, 0x00, sizeof *ctx);
	(void) EVP_MD_CTX_init(&ctx->state.d);

	if (! (ctx->md = digest_decide_md(alg)))
		return false;

	if (EVP_DigestInit_ex(&ctx->state.d, ctx->md, NULL) != 1)
		return false;

	return true;
}

bool ATHEME_FATTR_WUR
digest_init_hmac(struct digest_context *const restrict ctx, const unsigned int alg,
                 const void *const restrict key, const size_t keyLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}

	(void) memset(ctx, 0x00, sizeof *ctx);
	(void) HMAC_CTX_init(&ctx->state.h);

	ctx->hmac = true;

	if (! (ctx->md = digest_decide_md(alg)))
		return false;

	if (HMAC_Init_ex(&ctx->state.h, key, (int) keyLen, ctx->md, NULL) != 1)
		return false;

	return true;
}

bool ATHEME_FATTR_WUR
digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", __func__);
		return false;
	}

	if (! (data && dataLen))
		return true;

	if (ctx->hmac)
	{
		if (HMAC_Update(&ctx->state.h, data, dataLen) != 1)
			return false;
	}
	else
	{
		if (EVP_DigestUpdate(&ctx->state.d, data, dataLen) != 1)
			return false;
	}

	return true;
}

bool ATHEME_FATTR_WUR
digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", __func__);
		return false;
	}

	const size_t hLen = (size_t) EVP_MD_size(ctx->md);
	unsigned int uLen = EVP_MAX_MD_SIZE;

	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", __func__);
		return false;
	}
	else if (outLen)
		uLen = *outLen;

	if (ctx->hmac)
	{
		if (HMAC_Final(&ctx->state.h, out, &uLen) != 1)
			return false;

		(void) HMAC_CTX_cleanup(&ctx->state.h);
	}
	else
	{
		if (EVP_DigestFinal_ex(&ctx->state.d, out, &uLen) != 1)
			return false;

		(void) EVP_MD_CTX_cleanup(&ctx->state.d);
	}

	if (outLen)
		*outLen = (size_t) uLen;

	return true;
}

bool ATHEME_FATTR_WUR
digest_pbkdf2_hmac(const unsigned int alg, const void *restrict pass, const size_t passLen,
                   const void *restrict salt, const size_t saltLen, const size_t c,
                   void *const restrict dk, const size_t dkLen)
{
	if (! c)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'c' (BUG)", __func__);
		goto error;
	}
	if (! dk)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'dk' (BUG)", __func__);
		goto error;
	}
	if (! dkLen)
	{
		(void) slog(LG_ERROR, "%s: called with zero 'dkLen' (BUG)", __func__);
		goto error;
	}
	if ((! pass && passLen) || (pass && ! passLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched pass parameters (BUG)", __func__);
		goto error;
	}
	if ((! salt && saltLen) || (salt && ! saltLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched salt parameters (BUG)", __func__);
		goto error;
	}

	const EVP_MD *const md = digest_decide_md(alg);

	if (! md)
		goto error;

	/*
	 * PKCS5_PBKDF2_HMAC() fails if you give it a NULL argument for pass
	 * or salt, even if the corresponding length argument is zero! This
	 * is extremely counter-intuitive, and requires these ugly hacks.
	 */
	if (! pass)
		pass = &passLen;
	if (! salt)
		salt = &saltLen;

	if (PKCS5_PBKDF2_HMAC(pass, (int) passLen, salt, (int) saltLen, (int) c, md, (int) dkLen, dk) != 1)
		goto error;

	return true;

error:
	(void) smemzero(dk, dkLen);
	return false;
}
