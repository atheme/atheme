/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * OpenSSL frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_IMPL_H
#define ATHEME_INC_DIGEST_IMPL_H 1

#include <atheme/digest/types.h>
#include <atheme/stdheaders.h>

#include <openssl/evp.h>

struct digest_context
{
	const EVP_MD *          md;
	void *                  ictx;
	enum digest_algorithm   alg;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_IMPL_H */
