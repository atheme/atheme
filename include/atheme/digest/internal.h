/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Internal frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_IMPL_H
#define ATHEME_INC_DIGEST_IMPL_H 1

#include <atheme/digest/direct.h>
#include <atheme/digest/types.h>
#include <atheme/stdheaders.h>

struct digest_context
{
	union digest_direct_ctx     state;
	void                      (*init)(union digest_direct_ctx *);
	void                      (*update)(union digest_direct_ctx *, const void *, size_t);
	void                      (*final)(union digest_direct_ctx *, void *);
	size_t                      blksz;
	size_t                      digsz;
	unsigned char               ikey[DIGEST_BKLEN_MAX];
	unsigned char               okey[DIGEST_BKLEN_MAX];
	enum digest_algorithm       alg;
	bool                        hmac;
};

#endif /* !ATHEME_INC_DIGEST_IMPL_H */
