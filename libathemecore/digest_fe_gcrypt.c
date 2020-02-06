/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * GNU libgcrypt frontend for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

#ifndef GCRYPT_HEADER_INCL
#  define GCRYPT_NO_DEPRECATED 1
#  define GCRYPT_NO_MPI_MACROS 1
#  include <gcrypt.h>
#endif

const char *
digest_get_frontend_info(void)
{
	const char *const comverstr = GCRYPT_VERSION;
	const char *const libverstr = gcry_check_version(GCRYPT_VERSION);

	static char result[BUFSIZE];
	(void) snprintf(result, sizeof result, "GNU libgcrypt (compiled %s, library %s)", comverstr, libverstr);

	return result;
}

static inline enum gcry_md_algos
_digest_decide_md(const enum digest_algorithm alg)
{
	switch (alg)
	{
		case DIGALG_MD5:
			return GCRY_MD_MD5;

		case DIGALG_SHA1:
			return GCRY_MD_SHA1;

		case DIGALG_SHA2_256:
			return GCRY_MD_SHA256;

		case DIGALG_SHA2_512:
			return GCRY_MD_SHA512;
	}

	(void) slog(LG_ERROR, "%s: unrecognised algorithm '%u' (BUG)", MOWGLI_FUNC_NAME, (unsigned int) alg);
	return GCRY_MD_NONE;
}

static bool ATHEME_FATTR_WUR
_digest_init(struct digest_context *const restrict ctx, const enum digest_algorithm alg)
{
	(void) memset(ctx, 0x00, sizeof *ctx);

	ctx->alg = alg;

	if ((ctx->md = _digest_decide_md(alg)) == GCRY_MD_NONE)
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (gcry_md_test_algo(ctx->md) != 0)
	{
		(void) slog(LG_ERROR, "%s: unsupported algorithm '%u' (BUG)", MOWGLI_FUNC_NAME, (unsigned int) alg);
		return false;
	}
	if (gcry_md_open(&ctx->state, ctx->md, GCRY_MD_FLAG_SECURE) != 0)
	{
		(void) slog(LG_ERROR, "%s: gcry_md_open() failed (BUG?)", MOWGLI_FUNC_NAME);
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

	if ((ctx->md = _digest_decide_md(alg)) == GCRY_MD_NONE)
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (gcry_md_test_algo(ctx->md) != 0)
	{
		(void) slog(LG_ERROR, "%s: unsupported algorithm '%u' (BUG)", MOWGLI_FUNC_NAME, (unsigned int) alg);
		return false;
	}
	if (gcry_md_open(&ctx->state, ctx->md, GCRY_MD_FLAG_HMAC | GCRY_MD_FLAG_SECURE) != 0)
	{
		(void) slog(LG_ERROR, "%s: gcry_md_open() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (gcry_md_setkey(ctx->state, key, keyLen) != 0)
	{
		(void) slog(LG_ERROR, "%s: gcry_md_setkey() failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) gcry_md_close(ctx->state);
		return false;
	}

	return true;
}

static bool
_digest_update(struct digest_context *const restrict ctx, const void *const restrict data, const size_t dataLen)
{
	if (data && dataLen)
		(void) gcry_md_write(ctx->state, data, dataLen);

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_final(struct digest_context *const restrict ctx, void *const restrict out, size_t *const restrict outLen)
{
	const size_t hLen = digest_size_ctx(ctx);

	if (outLen)
		*outLen = hLen;

	const unsigned char *const digest = gcry_md_read(ctx->state, ctx->md);

	if (digest)
		(void) memcpy(out, digest, hLen);
	else
		(void) slog(LG_ERROR, "%s: gcry_md_read() failed (BUG?)", MOWGLI_FUNC_NAME);

	(void) gcry_md_close(ctx->state);

	return (digest != NULL);
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_pbkdf2(const enum digest_algorithm alg, const void *const restrict pass, const size_t passLen,
                       const void *const restrict salt, const size_t saltLen, const size_t c,
                       void *const restrict dk, const size_t dkLen)
{
	const enum gcry_md_algos md = _digest_decide_md(alg);

	if (md == GCRY_MD_NONE)
	{
		(void) slog(LG_ERROR, "%s: _digest_decide_md() failed (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if (gcry_md_test_algo(md) != 0)
	{
		(void) slog(LG_ERROR, "%s: unsupported algorithm '%u' (BUG)", MOWGLI_FUNC_NAME, (unsigned int) alg);
		return false;
	}
	if (gcry_kdf_derive(pass, passLen, GCRY_KDF_PBKDF2, md, salt, saltLen, c, dkLen, dk) != 0)
	{
		(void) slog(LG_ERROR, "%s: gcry_kdf_derive() failed (BUG?)", MOWGLI_FUNC_NAME);
		return false;
	}

	return true;
}
