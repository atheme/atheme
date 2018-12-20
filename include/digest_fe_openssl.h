/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * OpenSSL frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_FE_HEADER_H
#define ATHEME_INC_DIGEST_FE_HEADER_H 1

#include <openssl/evp.h>
#include <openssl/hmac.h>

struct digest_context
{
	const EVP_MD *          md;
	void *                  ictx;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_FE_HEADER_H */
