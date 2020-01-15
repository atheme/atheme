/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Do NOT increase the maximum values for hash and salt length below. These
 * were carefully-chosen to balance compatibility (with other implementations)
 * and usability (with this software).
 *
 * They also coincide with much of the recommendations of [1], in particular
 * the maximum thread count and maximum salt length.
 *
 * Higher values will likely result in exceeding the PASSLEN limit:
 *
 *     strlen("$argon2id$v=255$m=1073741824,t=1048576,p=255$$") [46]
 *     + BASE64_SIZE_RAW(ATHEME_ARGON2_SALTLEN_MAX [48]) [64]
 *     + BASE64_SIZE_RAW(ATHEME_ARGON2_HASHLEN_MAX [128]) [172]
 *     == 282
 *
 * Note that these restrictions do not apply to /verifying/ a password hash,
 * only computing a new one, so password hashes from other sources are allowed
 * to exceed these limits (assuming the string fits into the database to begin
 * with). However, the salt and hash lengths must still be at least the
 * /minimum/ values here.   -- amdj
 *
 * [1] <https://github.com/P-H-C/phc-string-format/blob/master/phc-sf-spec.md>
 */

#ifndef ATHEME_INC_ARGON2_H
#define ATHEME_INC_ARGON2_H 1

#define ATHEME_ARGON2_MEMCOST_MIN   3U
#define ATHEME_ARGON2_MEMCOST_DEF   16U
#define ATHEME_ARGON2_MEMCOST_MAX   30U

#define ATHEME_ARGON2_TIMECOST_MIN  3U
#define ATHEME_ARGON2_TIMECOST_DEF  3U
#define ATHEME_ARGON2_TIMECOST_MAX  1048576U

#define ATHEME_ARGON2_THREADS_MIN   1U
#define ATHEME_ARGON2_THREADS_DEF   1U
#define ATHEME_ARGON2_THREADS_MAX   255U

#define ATHEME_ARGON2_SALTLEN_MIN   4U
#define ATHEME_ARGON2_SALTLEN_DEF   16U
#define ATHEME_ARGON2_SALTLEN_MAX   48U

#define ATHEME_ARGON2_HASHLEN_MIN   12U
#define ATHEME_ARGON2_HASHLEN_DEF   64U
#define ATHEME_ARGON2_HASHLEN_MAX   128U

#endif /* !ATHEME_INC_ARGON2_H */
