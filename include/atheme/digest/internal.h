/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Internal frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_IMPL_H
#define ATHEME_INC_DIGEST_IMPL_H 1

#ifndef ATHEME_INC_DIGEST_H
#  error "You should not include me directly; include <atheme/digest.h> instead"
#endif /* !ATHEME_INC_DIGEST_H */

#include <atheme/digest/constants.h>
#include <atheme/stdheaders.h>

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
	struct digest_context_md5       md5_ctx;
	struct digest_context_sha1      sha1_ctx;
	struct digest_context_sha2_256  sha2_256_ctx;
	struct digest_context_sha2_512  sha2_512_ctx;
};

struct digest_context
{
	union digest_state      state;
	void                  (*init)(union digest_state *);
	void                  (*update)(union digest_state *, const void *, size_t);
	void                  (*final)(union digest_state *, void *);
	size_t                  blksz;
	size_t                  digsz;
	unsigned char           ikey[DIGEST_BKLEN_MAX];
	unsigned char           okey[DIGEST_BKLEN_MAX];
	enum digest_algorithm   alg;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_IMPL_H */
