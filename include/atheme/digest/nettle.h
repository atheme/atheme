/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Nettle frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_IMPL_H
#define ATHEME_INC_DIGEST_IMPL_H 1

#ifndef ATHEME_INC_DIGEST_H
#  error "You should not include me directly; include <atheme/digest.h> instead"
#endif /* !ATHEME_INC_DIGEST_H */

#include <atheme/digest/constants.h>
#include <atheme/stdheaders.h>

#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>

#include <nettle/nettle-meta.h>

union digest_state
{
	struct md5_ctx          md5_ctx;
	struct sha1_ctx         sha1_ctx;
	struct sha256_ctx       sha2_256_ctx;
	struct sha512_ctx       sha2_512_ctx;
};

typedef void (*digest_init_fn)(union digest_state *);
typedef void (*digest_update_fn)(union digest_state *, size_t, const void *);
typedef void (*digest_final_fn)(union digest_state *, size_t, void *);

struct digest_context
{
	union digest_state      state;
	digest_init_fn          init;
	digest_update_fn        update;
	digest_final_fn         final;
	size_t                  blksz;
	size_t                  digsz;
	unsigned char           ikey[DIGEST_BKLEN_MAX];
	unsigned char           okey[DIGEST_BKLEN_MAX];
	enum digest_algorithm   alg;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_IMPL_H */
