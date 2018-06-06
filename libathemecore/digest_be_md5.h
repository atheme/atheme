/*
 * SPDX-License-Identifier: CC0-1.0
 * SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
 *
 * MD5 backend function declarations for the internal digest frontend.
 */

#ifndef ATHEME_LAC_DIGEST_BE_MD5_H
#define ATHEME_LAC_DIGEST_BE_MD5_H 1

bool digest_init_md5(struct digest_context_md5 *);
bool digest_update_md5(struct digest_context_md5 *, const void *, size_t);
bool digest_final_md5(struct digest_context_md5 *, void *, size_t *);

#endif /* !ATHEME_LAC_DIGEST_BE_MD5_H */
