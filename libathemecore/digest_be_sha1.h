/*
 * SPDX-License-Identifier: CC0-1.0
 * SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
 *
 * SHA1 backend function declarations for the internal digest frontend.
 */

#ifndef ATHEME_LAC_DIGEST_BE_SHA1_H
#define ATHEME_LAC_DIGEST_BE_SHA1_H 1

bool digest_init_sha1(struct digest_context_sha1 *);
bool digest_update_sha1(struct digest_context_sha1 *, const void *, size_t);
bool digest_final_sha1(struct digest_context_sha1 *, void *, size_t *);

#endif /* !ATHEME_LAC_DIGEST_BE_SHA1_H */
