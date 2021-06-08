/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2017 Aaron M. D. Jones <me@aaronmdjones.net>
 */

#ifndef ATHEME_INC_BASE64_H
#define ATHEME_INC_BASE64_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

#define BASE64_ALPHABET_RFC4648         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
#define BASE64_ALPHABET_RFC4648_NOPAD   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
#define BASE64_ALPHABET_CRYPT3          "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define BASE64_ALPHABET_CRYPT3_BLOWFISH "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

// This is returned when the encoder or decoder fails
#define BASE64_FAIL             ((size_t) -1)

/* This macro computes the base64 buffer size requirement for encoding a
 * given length of input. This is complicated by the fact that termination
 * padding is usually required if the input is not a multiple of three
 * bytes. Specifically, if (N % 3) is equal to 1, there will be two padding
 * characters, and if it is equal to 2, there will be one padding character.
 * In all cases, the output will be a multiple of four characters, but the
 * requirement to account for padding means any division has to round up,
 * whereas integer division in C rounds toward zero.
 *
 * This macro evaluates its input expression only once, results in a
 * compile-time constant result (for a compile-time constant input),
 * ensures that the division rounds up instead of toward zero, does not
 * over-estimate the size of the buffer required, and finally adds 1 to
 * the result, for a C string termination NULL byte. A raw variant is
 * provided that does not account for the termination.
 *
 *    -- amdj
 */
#define BASE64_SIZE_RAW(len)    ((((len) + 2U) / 3U) * 4U)
#define BASE64_SIZE_STR(len)    (BASE64_SIZE_RAW(len) + 1U)

size_t base64_decode(const char *, void *, size_t) ATHEME_FATTR_WUR;
size_t base64_decode_table(const char *, void *, size_t, const char alphabet[static 65]) ATHEME_FATTR_WUR;

size_t base64_encode(const void *, size_t, char *, size_t) ATHEME_FATTR_WUR;
size_t base64_encode_table(const void *, size_t, char *, size_t, const char alphabet[static 65]) ATHEME_FATTR_WUR;

#endif /* !ATHEME_INC_BASE64_H */
