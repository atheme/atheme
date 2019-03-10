/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Internal frontend data structures for the digest interface.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_DIGEST_FE_HEADER_H
#define ATHEME_INC_DIGEST_FE_HEADER_H 1

#ifndef ATHEME_INC_DIGEST_H
#  error "You should not include me directly; include digest.h instead"
#endif /* !ATHEME_INC_DIGEST_H */

#include "attrs.h"
#include "digest.h"
#include "structures.h"

#define DIGEST_STLEN_MD5        0x04U
#define DIGEST_STLEN_SHA1       0x05U
#define DIGEST_STLEN_SHA2       0x08U

struct digest_context_md5
{
	uint32_t        count[0x02U];
	uint32_t        state[DIGEST_STLEN_MD5];
	unsigned char   buf[DIGEST_BKLEN_MD5];
};

struct digest_context_sha1
{
	uint32_t        count[0x02U];
	uint32_t        state[DIGEST_STLEN_SHA1];
	unsigned char   buf[DIGEST_BKLEN_SHA1];
};

struct digest_context_sha2_256
{
	uint64_t        count;
	uint32_t        state[DIGEST_STLEN_SHA2];
	unsigned char   buf[DIGEST_BKLEN_SHA2_256];
};

struct digest_context_sha2_512
{
	uint64_t        count[0x02U];
	uint64_t        state[DIGEST_STLEN_SHA2];
	unsigned char   buf[DIGEST_BKLEN_SHA2_512];
};

union digest_state
{
	struct digest_context_md5       md5ctx;
	struct digest_context_sha1      sha1ctx;
	struct digest_context_sha2_256  sha256ctx;
	struct digest_context_sha2_512  sha512ctx;
};

typedef bool (*digest_init_fn)(union digest_state *) ATHEME_FATTR_WUR;
typedef bool (*digest_update_fn)(union digest_state *, const void *, size_t) ATHEME_FATTR_WUR;
typedef bool (*digest_final_fn)(union digest_state *, void *, size_t *) ATHEME_FATTR_WUR;

struct digest_context
{
	union digest_state      state;
	unsigned char           ikey[DIGEST_BKLEN_MAX];
	unsigned char           okey[DIGEST_BKLEN_MAX];
	digest_init_fn          init;
	digest_update_fn        update;
	digest_final_fn         final;
	size_t                  blksz;
	size_t                  digsz;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_FE_HEADER_H */
