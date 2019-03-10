/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Shared routines for the digest interface.
 */

#ifndef ATHEME_LAC_DIGEST_FRONTEND_C
#  error "Do not compile me directly; compile digest_frontend.c instead"
#endif /* !ATHEME_LAC_DIGEST_FRONTEND_C */

static bool ATHEME_FATTR_WUR
_digest_update_vector(struct digest_context *const restrict ctx, const struct digest_vector *const restrict vec,
                      const size_t vecLen)
{
	for (size_t i = 0; i < vecLen; i++)
		if (! _digest_update(ctx, vec[i].ptr, vec[i].len))
			return false;

	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot(const enum digest_algorithm alg, const void *const data, const size_t dataLen,
                void *const out, size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init(&ctx, alg))
		return false;

	if (! _digest_update(&ctx, data, dataLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_vector(const enum digest_algorithm alg, const struct digest_vector *const restrict vec,
                       const size_t vecLen, void *const restrict out, size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init(&ctx, alg))
		return false;

	if (! _digest_update_vector(&ctx, vec, vecLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_hmac(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                     const void *const data, const size_t dataLen, void *const out,
                     size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	if (! _digest_update(&ctx, data, dataLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

static bool ATHEME_FATTR_WUR
_digest_oneshot_hmac_vector(const enum digest_algorithm alg, const void *const key, const size_t keyLen,
                            const struct digest_vector *const restrict vec, const size_t vecLen,
                            void *const out, size_t *const outLen)
{
	struct digest_context ctx;

	if (! _digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	if (! _digest_update_vector(&ctx, vec, vecLen))
		return false;

	if (! _digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}
