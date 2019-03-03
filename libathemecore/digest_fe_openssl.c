/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * OpenSSL frontend for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>

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
digest_free_internal(struct digest_context *const restrict ctx)
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
digest_decide_md(const enum digest_algorithm alg)
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

const char *
digest_get_frontend_info(void)
{
	return OPENSSL_VERSION_TEXT;
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

	if (! (ctx->ictx = EVP_MD_CTX_create()))
	{
		(void) slog(LG_ERROR, "%s: EVP_MD_CTX_create(3): unknown error", MOWGLI_FUNC_NAME);
		return false;
	}
	if (EVP_DigestInit_ex(ctx->ictx, ctx->md, NULL) != 1)
	{
		(void) slog(LG_ERROR, "%s: EVP_DigestInit_ex(3): unknown error", MOWGLI_FUNC_NAME);
		(void) digest_free_internal(ctx);
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

	ctx->hmac = true;

	if (! (ctx->md = digest_decide_md(alg)))
		return false;

	if (! (ctx->ictx = HMAC_CTX_new()))
	{
		(void) slog(LG_ERROR, "%s: HMAC_CTX_new(3): unknown error", MOWGLI_FUNC_NAME);
		return false;
	}
	if (HMAC_Init_ex(ctx->ictx, key, (int) keyLen, ctx->md, NULL) != 1)
	{
		(void) slog(LG_ERROR, "%s: HMAC_Init_ex(3): unknown error", MOWGLI_FUNC_NAME);
		(void) digest_free_internal(ctx);
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
		goto error;
	}
	if (! ctx->ictx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx->ictx' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}

	if (! (data && dataLen))
		return true;

	if (ctx->hmac)
	{
		if (HMAC_Update(ctx->ictx, data, dataLen) != 1)
			goto error;
	}
	else
	{
		if (EVP_DigestUpdate(ctx->ictx, data, dataLen) != 1)
			goto error;
	}

	return true;

error:
	(void) digest_free_internal(ctx);

	return false;
}

bool ATHEME_FATTR_WUR
digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! ctx->ictx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx->ictx' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}

	const size_t hLen = (size_t) EVP_MD_size(ctx->md);
	unsigned int uLen = EVP_MAX_MD_SIZE;

	if (outLen && *outLen < hLen)
	{
		(void) slog(LG_ERROR, "%s: output buffer is too small (BUG)", MOWGLI_FUNC_NAME);
		goto error;
	}
	else if (outLen)
		uLen = *outLen;

	if (ctx->hmac)
	{
		if (HMAC_Final(ctx->ictx, out, &uLen) != 1)
			goto error;
	}
	else
	{
		if (EVP_DigestFinal_ex(ctx->ictx, out, &uLen) != 1)
			goto error;
	}

	if (outLen)
		*outLen = (size_t) uLen;

	(void) digest_free_internal(ctx);

	return true;

error:
	(void) digest_free_internal(ctx);

	return false;
}

bool ATHEME_FATTR_WUR
digest_oneshot_pbkdf2(const enum digest_algorithm alg, const void *restrict pass, const size_t passLen,
                      const void *restrict salt, const size_t saltLen, const size_t c,
                      void *const restrict dk, const size_t dkLen)
{
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

	const EVP_MD *const md = digest_decide_md(alg);

	if (! md)
		goto error;

	/*
	 * PKCS5_PBKDF2_HMAC() fails if you give it a NULL argument for pass
	 * or salt, even if the corresponding length argument is zero! This
	 * is extremely counter-intuitive, and requires these ugly hacks.  -- amdj
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
