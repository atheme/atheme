/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * SHA2 backend function declarations for the internal digest frontend.
 */

#ifndef ATHEME_LAC_DIGEST_BE_SHA2_H
#define ATHEME_LAC_DIGEST_BE_SHA2_H 1

bool digest_init_sha2_256(struct digest_context_sha2_256 *);
bool digest_update_sha2_256(struct digest_context_sha2_256 *, const void *, size_t);
bool digest_final_sha2_256(struct digest_context_sha2_256 *, void *, size_t *);

bool digest_init_sha2_512(struct digest_context_sha2_512 *);
bool digest_update_sha2_512(struct digest_context_sha2_512 *, const void *, size_t);
bool digest_final_sha2_512(struct digest_context_sha2_512 *, void *, size_t *);

#endif /* !ATHEME_LAC_DIGEST_BE_SHA2_H */
