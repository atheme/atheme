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

union digest_state
{
	EVP_MD_CTX      d;
	HMAC_CTX        h;
};

struct digest_context
{
	union digest_state      state;
	const EVP_MD *          md;
	bool                    hmac;
};

#endif /* !ATHEME_INC_DIGEST_FE_HEADER_H */
