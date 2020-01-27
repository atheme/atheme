/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Atheme IRC Services digest interface.
 */

#ifndef ATHEME_INC_DIGEST_DIRECT_H
#define ATHEME_INC_DIGEST_DIRECT_H 1

#include <atheme/stdheaders.h>

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

struct digest_direct_ctx_md5
{
	uint32_t        count[0x02U];
	uint32_t        state[DIGEST_IVLEN_MD5];
	unsigned char   buf[DIGEST_BKLEN_MD5];
};

struct digest_direct_ctx_sha1
{
	uint32_t        count[0x02U];
	uint32_t        state[DIGEST_IVLEN_SHA1];
	unsigned char   buf[DIGEST_BKLEN_SHA1];
};

struct digest_direct_ctx_sha2_256
{
	uint64_t        count;
	uint32_t        state[DIGEST_IVLEN_SHA2_256];
	unsigned char   buf[DIGEST_BKLEN_SHA2_256];
};

struct digest_direct_ctx_sha2_512
{
	uint64_t        count[0x02U];
	uint64_t        state[DIGEST_IVLEN_SHA2_512];
	unsigned char   buf[DIGEST_BKLEN_SHA2_512];
};

union digest_direct_ctx
{
	struct digest_direct_ctx_md5        md5;
	struct digest_direct_ctx_sha1       sha1;
	struct digest_direct_ctx_sha2_256   sha2_256;
	struct digest_direct_ctx_sha2_512   sha2_512;
};

void digest_direct_init_md5(union digest_direct_ctx *);
void digest_direct_init_sha1(union digest_direct_ctx *);
void digest_direct_init_sha2_256(union digest_direct_ctx *);
void digest_direct_init_sha2_512(union digest_direct_ctx *);

void digest_direct_update_md5(union digest_direct_ctx *, const void *, size_t);
void digest_direct_update_sha1(union digest_direct_ctx *, const void *, size_t);
void digest_direct_update_sha2_256(union digest_direct_ctx *, const void *, size_t);
void digest_direct_update_sha2_512(union digest_direct_ctx *, const void *, size_t);

void digest_direct_final_md5(union digest_direct_ctx *, void *);
void digest_direct_final_sha1(union digest_direct_ctx *, void *);
void digest_direct_final_sha2_256(union digest_direct_ctx *, void *);
void digest_direct_final_sha2_512(union digest_direct_ctx *, void *);

#endif /* !ATHEME_INC_DIGEST_DIRECT_H */
