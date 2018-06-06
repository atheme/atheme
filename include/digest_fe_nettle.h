/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Nettle frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_FE_HEADER_H
#define ATHEME_INC_DIGEST_FE_HEADER_H 1

#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>

#include <nettle/nettle-meta.h>

union digest_state
{
	struct md5_ctx          md5ctx;
	struct sha1_ctx         sha1ctx;
	struct sha256_ctx       sha256ctx;
	struct sha512_ctx       sha512ctx;
};

typedef void (*digest_init_fn)(union digest_state *);
typedef void (*digest_update_fn)(union digest_state *, size_t, const void *);
typedef void (*digest_final_fn)(union digest_state *, size_t, void *);

struct digest_context
{
	union digest_state      state;
	uint8_t                 ikey[DIGEST_BKLEN_MAX];
	uint8_t                 okey[DIGEST_BKLEN_MAX];
	digest_init_fn          init;
	digest_update_fn        update;
	digest_final_fn         final;
	size_t                  blksz;
	size_t                  digsz;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_FE_HEADER_H */
