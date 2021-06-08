/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Atheme IRC Services digest interface.
 */

#ifndef ATHEME_INC_DIGEST_H
#define ATHEME_INC_DIGEST_H 1

#include <atheme/attributes.h>
#include <atheme/digest/direct.h>
#include <atheme/digest/types.h>
#include <atheme/stdheaders.h>

#if (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_OPENSSL)
#  include <atheme/digest/openssl.h>
#elif (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_GCRYPT)
#  include <atheme/digest/gcrypt.h>
#elif (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_MBEDTLS)
#  include <atheme/digest/mbedtls.h>
#elif (ATHEME_API_DIGEST_FRONTEND == ATHEME_API_DIGEST_FRONTEND_INTERNAL)
#  include <atheme/digest/internal.h>
#else
#  error "No Digest API frontend was selected by the build system"
#endif

#ifndef ATHEME_INC_DIGEST_IMPL_H
#  error "No Digest API frontend header is available"
#endif /* !ATHEME_INC_DIGEST_IMPL_H */

size_t digest_size_alg(enum digest_algorithm);
size_t digest_size_ctx(const struct digest_context *);

bool digest_init(struct digest_context *, enum digest_algorithm) ATHEME_FATTR_WUR;
bool digest_init_hmac(struct digest_context *, enum digest_algorithm, const void *, size_t) ATHEME_FATTR_WUR;
bool digest_update(struct digest_context *, const void *, size_t) ATHEME_FATTR_WUR;
bool digest_update_vector(struct digest_context *, const struct digest_vector *, size_t) ATHEME_FATTR_WUR;
bool digest_final(struct digest_context *, void *, size_t *) ATHEME_FATTR_WUR;

bool digest_oneshot(enum digest_algorithm, const void *, size_t, void *, size_t *) ATHEME_FATTR_WUR;
bool digest_oneshot_vector(enum digest_algorithm, const struct digest_vector *, size_t, void *, size_t *)
    ATHEME_FATTR_WUR;
bool digest_oneshot_hmac(enum digest_algorithm, const void *, size_t, const void *, size_t, void *, size_t *)
    ATHEME_FATTR_WUR;
bool digest_oneshot_hmac_vector(enum digest_algorithm, const void *, size_t, const struct digest_vector *, size_t,
    void *, size_t *) ATHEME_FATTR_WUR;

bool digest_hkdf_extract(enum digest_algorithm, const void *, size_t, const void *, size_t, void *, size_t)
    ATHEME_FATTR_WUR;
bool digest_hkdf_expand(enum digest_algorithm, const void *, size_t, const void *, size_t, void *, size_t)
    ATHEME_FATTR_WUR;
bool digest_oneshot_hkdf(enum digest_algorithm, const void *, size_t, const void *, size_t, const void *, size_t,
    void *, size_t) ATHEME_FATTR_WUR;

bool digest_oneshot_pbkdf2(enum digest_algorithm, const void *, size_t, const void *, size_t, size_t, void *, size_t)
    ATHEME_FATTR_WUR;

bool digest_testsuite_run(void) ATHEME_FATTR_WUR;
const char *digest_get_frontend_info(void);

#endif /* !ATHEME_INC_DIGEST_H */
