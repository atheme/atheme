/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the digest interface.
 */

#include "atheme.h"

#if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_INTERNAL)
#  include "digest_fe_internal.c"
#else
#  if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_MBEDTLS)
#    include "digest_fe_mbedtls.c"
#  else
#    if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_NETTLE)
#      include "digest_fe_nettle.c"
#    else
#      if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_OPENSSL)
#        include "digest_fe_openssl.c"
#      else
#        error "No digest frontend"
#      endif
#    endif
#  endif
#endif

bool ATHEME_FATTR_WUR
digest_oneshot(const unsigned int alg, const void *const restrict data, const size_t dataLen,
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
digest_oneshot_hmac(const unsigned int alg, const void *const restrict key, const size_t keyLen,
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
