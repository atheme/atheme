/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the digest interface.
 */

#include "atheme.h"

#define ATHEME_LAC_DIGEST_FRONTEND_C    1

#if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_INTERNAL)
#  include "digest_fe_internal.c"
#else
#  if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_MBEDTLS)
#    include "digest_fe_mbedtls.c"
#  else
#    if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_NETTLE)
#      include "digest_fe_nettle.c"
#    else
#      if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_OPENSSL)
#        include "digest_fe_openssl.c"
#      else
#        error "No Digest API frontend was selected by the build system"
#      endif
#    endif
#  endif
#endif

bool ATHEME_FATTR_WUR
digest_update_vector(struct digest_context *const restrict ctx, const struct digest_vector *const restrict vec,
                     const size_t vecLen)
{
	if (! ctx)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'ctx' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! vec && vecLen) || (vec && ! vecLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	for (size_t i = 0; i < vecLen; i++)
	{
		if ((! vec[i].ptr && vec[i].len) || (vec[i].ptr && ! vec[i].len))
		{
			(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	for (size_t i = 0; i < vecLen; i++)
		if (! digest_update(ctx, vec[i].ptr, vec[i].len))
			return false;

	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_vector(const enum digest_algorithm alg, const struct digest_vector *const restrict vec,
                      const size_t vecLen, void *const restrict out, size_t *const restrict outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! vec && vecLen) || (vec && ! vecLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	for (size_t i = 0; i < vecLen; i++)
	{
		if ((! vec[i].ptr && vec[i].len) || (vec[i].ptr && ! vec[i].len))
		{
			(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	struct digest_context ctx;

	if (! digest_init(&ctx, alg))
		return false;

	for (size_t i = 0; i < vecLen; i++)
		if (! digest_update(&ctx, vec[i].ptr, vec[i].len))
			return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_hmac_vector(const enum digest_algorithm alg, const void *const restrict key, const size_t keyLen,
                           const struct digest_vector *const restrict vec, const size_t vecLen,
                           void *const restrict out, size_t *const restrict outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! key && keyLen) || (key && ! keyLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched key parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! vec && vecLen) || (vec && ! vecLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	for (size_t i = 0; i < vecLen; i++)
	{
		if ((! vec[i].ptr && vec[i].len) || (vec[i].ptr && ! vec[i].len))
		{
			(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
			return false;
		}
	}

	struct digest_context ctx;

	if (! digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	for (size_t i = 0; i < vecLen; i++)
		if (! digest_update(&ctx, vec[i].ptr, vec[i].len))
			return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot(const enum digest_algorithm alg, const void *const restrict data, const size_t dataLen,
               void *const restrict out, size_t *const restrict outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	struct digest_context ctx;

	if (! digest_init(&ctx, alg))
		return false;

	if (! digest_update(&ctx, data, dataLen))
		return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}

bool ATHEME_FATTR_WUR
digest_oneshot_hmac(const enum digest_algorithm alg, const void *const restrict key, const size_t keyLen,
                    const void *const restrict data, const size_t dataLen, void *const restrict out,
                    size_t *const restrict outLen)
{
	if (! out)
	{
		(void) slog(LG_ERROR, "%s: called with NULL 'out' (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! key && keyLen) || (key && ! keyLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched key parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}
	if ((! data && dataLen) || (data && ! dataLen))
	{
		(void) slog(LG_ERROR, "%s: called with mismatched data parameters (BUG)", MOWGLI_FUNC_NAME);
		return false;
	}

	struct digest_context ctx;

	if (! digest_init_hmac(&ctx, alg, key, keyLen))
		return false;

	if (! digest_update(&ctx, data, dataLen))
		return false;

	if (! digest_final(&ctx, out, outLen))
		return false;

	(void) smemzero(&ctx, sizeof ctx);
	return true;
}
