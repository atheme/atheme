/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * GNU libgcrypt frontend data structures for the digest interface.
 */

#ifndef ATHEME_INC_DIGEST_IMPL_H
#define ATHEME_INC_DIGEST_IMPL_H 1

#include <atheme/digest/types.h>

#ifndef GCRYPT_HEADER_INCL
#  define GCRYPT_HEADER_INCL   1
#  define GCRYPT_NO_DEPRECATED 1
#  define GCRYPT_NO_MPI_MACROS 1
#  include <gcrypt.h>
#endif

struct digest_context
{
	gcry_md_hd_t            state;
	enum gcry_md_algos      md;
	enum digest_algorithm   alg;
};

#endif /* !ATHEME_INC_DIGEST_IMPL_H */
