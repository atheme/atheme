/*
 * ARM mbedTLS frontend for the digest interface.
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>

static inline const mbedtls_md_info_t *
digest_decide_md(const unsigned int alg)
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

		default:
			(void) slog(LG_ERROR, "%s: called with unknown/unimplemented alg '%u' (BUG)",
			                      __func__, alg);
			return NULL;
	}

	return mbedtls_md_info_from_type(md_type);
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
digest_init_hmac(struct digest_context *const restrict ctx, const unsigned int alg,
                 const void *const restrict key, const size_t keyLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
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
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", __func__);
		return false;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", __func__);
		return false;
	}

	const size_t hLen = (size_t) mbedtls_md_get_size(ctx->md);

	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", __func__);
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
digest_pbkdf2_hmac(const unsigned int alg, const void *const restrict pass, const size_t passLen,
                   const void *const restrict salt, const size_t saltLen, const size_t c,
                   void *const restrict dk, const size_t dkLen)
{
	mbedtls_md_context_t ctx;

	(void) mbedtls_md_init(&ctx);

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
