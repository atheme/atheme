/*
 * Atheme IRC Services digest interface.
 *
 * Copyright (C) 2018 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ATHEME_INC_DIGEST_H
#define ATHEME_INC_DIGEST_H 1

#include "attrs.h"
#include "sysconf.h"

#define DIGALG_MD5              0x01U
#define DIGALG_SHA1             0x02U
#define DIGALG_SHA2_256         0x03U
#define DIGALG_SHA2_512         0x04U

#define DIGEST_BKLEN_MD5        0x40U
#define DIGEST_MDLEN_MD5        0x10U

#define DIGEST_BKLEN_SHA1       0x40U
#define DIGEST_MDLEN_SHA1       0x14U

#define DIGEST_BKLEN_SHA2_256   0x40U
#define DIGEST_MDLEN_SHA2_256   0x20U

#define DIGEST_BKLEN_SHA2_512   0x80U
#define DIGEST_MDLEN_SHA2_512   0x40U

#define DIGEST_BKLEN_MAX        DIGEST_BKLEN_SHA2_512
#define DIGEST_MDLEN_MAX        DIGEST_MDLEN_SHA2_512

#if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_INTERNAL)
#  include "digest_fe_internal.h"
#else
#  if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_MBEDTLS)
#    include "digest_fe_mbedtls.h"
#  else
#    if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_NETTLE)
#      include "digest_fe_nettle.h"
#    else
#      if (ATHEME_DIGEST_FRONTEND == ATHEME_DIGEST_FRONTEND_OPENSSL)
#        include "digest_fe_openssl.h"
#      else
#        error "No digest frontend"
#      endif
#    endif
#  endif
#endif

#ifndef ATHEME_INC_DIGEST_FE_HEADER_H
#  error "No digest frontend"
#endif /* !ATHEME_INC_DIGEST_FE_HEADER_H */

bool digest_init(struct digest_context *, unsigned int) ATHEME_FATTR_WUR;
bool digest_init_hmac(struct digest_context *, unsigned int, const void *, size_t) ATHEME_FATTR_WUR;
bool digest_update(struct digest_context *, const void *, size_t) ATHEME_FATTR_WUR;
bool digest_final(struct digest_context *, void *, size_t *) ATHEME_FATTR_WUR;

bool digest_oneshot(unsigned int, const void *, size_t, void *, size_t *) ATHEME_FATTR_WUR;
bool digest_oneshot_hmac(unsigned int, const void *, size_t, const void *, size_t, void *, size_t *) ATHEME_FATTR_WUR;
bool digest_pbkdf2_hmac(unsigned int, const void *, size_t, const void *, size_t, size_t, void *, size_t) ATHEME_FATTR_WUR;

bool digest_testsuite_run(void) ATHEME_FATTR_WUR;
const char *digest_get_frontend_info(void);

#endif /* !ATHEME_INC_DIGEST_H */
