/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * OpenSSL frontend for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

const char *
digest_get_frontend_info(void)
{
	return OPENSSL_VERSION_TEXT;
}

#ifndef HAVE_LIBCRYPTO_HMAC_CTX_DYNAMIC

/*
 * Grumble. If you're (OpenSSL developers) going to stop exporting the
 * definitions of your internal structures and provide new/free functions
 * for your API instead, you really should do it for *all* versions of your
 * API.
 *
 * Seriously, guys.  --amdj
 */

static inline HMAC_CTX * ATHEME_FATTR_WUR
HMAC_CTX_new(void)
{
	HMAC_CTX *const ctx = smalloc(sizeof *ctx);

	(void) HMAC_CTX_init(ctx);

	return ctx;
}

static inline void
HMAC_CTX_free(HMAC_CTX *const restrict ctx)
{
	(void) HMAC_CTX_cleanup(ctx);
	(void) sfree(ctx);
}

#endif /* !HAVE_LIBCRYPTO_HMAC_CTX_DYNAMIC */

static inline void
_digest_free_internal(struct digest_context *const restrict ctx)
{
	if (! ctx || ! ctx->ictx)
		return;

	if (ctx->hmac)
		(void) HMAC_CTX_free(ctx->ictx);
	else
		(void) EVP_MD_CTX_destroy(ctx->ictx);

	ctx->ictx = NULL;
}

static inline const EVP_MD *
_digest_decide_md(const enum digest_algorithm alg)
{
	const EVP_MD *md = NULL;

	switch (alg)
	{
		case DIGALG_MD5:
			md = EVP_md5();
			break;

		case DIGALG_SHA1:
			md = EVP_sha1();
			break;

		case DIGALG_SHA2_256:
			md = EVP_sha256();
			break;

		case DIGALG_SHA2_512:
			md = EVP_sha512();
			break;
	}

	return md;
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
	if (! (ctx->ictx = EVP_MD_CTX_create()))
	{
		(void) slog(LG_ERROR, "%s: EVP_MD_CTX_create(3): unknown error", MOWGLI_FUNC_NAME);
		return false;
	}
	if (EVP_DigestInit_ex(ctx->ictx, ctx->md, NULL) != 1)
	{
		(void) slog(LG_ERROR, "%s: EVP_DigestInit_ex(3): unknown error", MOWGLI_FUNC_NAME);
		(void) _digest_free_internal(ctx);
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
	if (! (ctx->ictx = HMAC_CTX_new()))
	{
		(void) slog(LG_ERROR, "%s: HMAC_CTX_new(3): unknown error", MOWGLI_FUNC_NAME);
		return false;
	}
	if (HMAC_Init_ex(ctx->ictx, key, (int) keyLen, ctx->md, NULL) != 1)
	{
		(void) slog(LG_ERROR, "%s: HMAC_Init_ex(3): unknown error", MOWGLI_FUNC_NAME);
		(void) _digest_free_internal(ctx);
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
		if (HMAC_Update(ctx->ictx, data, dataLen) != 1)
		{
			(void) _digest_free_internal(ctx);
			return false;
		}
	}
	else
	{
		if (EVP_DigestUpdate(ctx->ictx, data, dataLen) != 1)
		{
			(void) _digest_free_internal(ctx);
			return false;
		}
	}

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	const size_t hLen = digest_size_ctx(ctx);
	unsigned int uLen = (unsigned int) hLen;
	int ret;

	if (outLen)
		*outLen = hLen;

	if (ctx->hmac)
		ret = HMAC_Final(ctx->ictx, out, &uLen);
	else
		ret = EVP_DigestFinal_ex(ctx->ictx, out, &uLen);

	(void) _digest_free_internal(ctx);

	return (ret == 1);
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_pbkdf2(const enum digest_algorithm alg, const void *const restrict pass, const size_t passLen,
                       const void *const restrict salt, const size_t saltLen, const size_t c,
                       void *const restrict dk, const size_t dkLen)
{
	const EVP_MD *const md = _digest_decide_md(alg);

	if (! md)
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}

	return (PKCS5_PBKDF2_HMAC(pass, (int) passLen, salt, (int) saltLen, (int) c, md, (int) dkLen, dk) == 1);
}
