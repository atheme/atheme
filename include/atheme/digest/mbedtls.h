/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * ARM mbedTLS frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_IMPL_H
#define ATHEME_INC_DIGEST_IMPL_H 1

#include <atheme/digest/types.h>
#include <atheme/stdheaders.h>

#include <mbedtls/md.h>

struct digest_context
{
	mbedtls_md_context_t            state;
	const mbedtls_md_info_t *       md;
	enum digest_algorithm           alg;
	bool                            hmac;
};

#endif /* !ATHEME_INC_DIGEST_IMPL_H */
