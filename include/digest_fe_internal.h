/*
 * Internal frontend data structures for the digest interface.
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ATHEME_INC_DIGEST_H
#  error "You should not include me directly; include digest.h instead"
#endif /* !ATHEME_INC_DIGEST_H */

#ifndef ATHEME_INC_DIGEST_FE_HEADER_H
#define ATHEME_INC_DIGEST_FE_HEADER_H 1

#include "attrs.h"

#define DIGEST_STLEN_MD5        0x04U
#define DIGEST_STLEN_SHA1       0x05U
#define DIGEST_STLEN_SHA2       0x08U

struct digest_context_md5
{
	uint32_t        count[0x02U];
	uint32_t        state[DIGEST_STLEN_MD5];
	uint8_t         buf[DIGEST_BKLEN_MD5];
};

struct digest_context_sha1
{
	uint32_t        count[0x02U];
	uint32_t        state[DIGEST_STLEN_SHA1];
	uint8_t         buf[DIGEST_BKLEN_SHA1];
};

struct digest_context_sha2_256
{
	uint64_t        count;
	uint32_t        state[DIGEST_STLEN_SHA2];
	uint8_t         buf[DIGEST_BKLEN_SHA2_256];
};

struct digest_context_sha2_512
{
	uint64_t        count[0x02U];
	uint64_t        state[DIGEST_STLEN_SHA2];
	uint8_t         buf[DIGEST_BKLEN_SHA2_512];
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
