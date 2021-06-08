/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Atheme IRC Services digest interface.
 */

#ifndef ATHEME_INC_DIGEST_TYPES_H
#define ATHEME_INC_DIGEST_TYPES_H 1

#include <atheme/stdheaders.h>

enum digest_algorithm
{
	DIGALG_MD5      = 1,
	DIGALG_SHA1     = 2,
	DIGALG_SHA2_256 = 3,
	DIGALG_SHA2_512 = 4,
};

struct digest_vector
{
	const void *    ptr;
	size_t          len;
};

#endif /* !ATHEME_INC_DIGEST_TYPES_H */
