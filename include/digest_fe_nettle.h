/*
 * Nettle frontend data structures for the digest interface.
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
