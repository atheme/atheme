/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 */

#ifndef ATHEME_INC_DIGEST_CONSTANTS_H
#define ATHEME_INC_DIGEST_CONSTANTS_H 1

enum digest_algorithm
{
	DIGALG_MD5              = 1,
	DIGALG_SHA1             = 2,
	DIGALG_SHA2_256         = 3,
	DIGALG_SHA2_512         = 4,
};

#define DIGEST_BKLEN_MD5        0x40U
#define DIGEST_IVLEN_MD5        0x04U
#define DIGEST_MDLEN_MD5        0x10U

#define DIGEST_BKLEN_SHA1       0x40U
#define DIGEST_IVLEN_SHA1       0x05U
#define DIGEST_MDLEN_SHA1       0x14U

#define DIGEST_BKLEN_SHA2_256   0x40U
#define DIGEST_IVLEN_SHA2_256   0x08U
#define DIGEST_MDLEN_SHA2_256   0x20U

#define DIGEST_BKLEN_SHA2_512   0x80U
#define DIGEST_IVLEN_SHA2_512   0x08U
#define DIGEST_MDLEN_SHA2_512   0x40U

#define DIGEST_BKLEN_MAX        DIGEST_BKLEN_SHA2_512
#define DIGEST_MDLEN_MAX        DIGEST_MDLEN_SHA2_512

#endif /* !ATHEME_INC_DIGEST_CONSTANTS_H */
