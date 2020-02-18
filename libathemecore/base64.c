/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2017 Aaron M. D. Jones <aaronmdjones@gmail.com>
 */

#include <atheme.h>
#include "internal.h"

static const char alphabet_default[] = {

	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
	0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x2F,
	0x3D,
};

static const unsigned char inverse_alphabet_default[] = {

	0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF,
	0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
	0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static bool ATHEME_FATTR_WUR
base64_alphabet_invert(const char alphabet[const restrict static 65],
                       unsigned char inverse_alphabet[const restrict static 128])
{
	// Prefill with invalid character indicator
	(void) memset(inverse_alphabet, 0xFF, 128);

	for (unsigned char i = 0; i < 65; i++)
	{
		if (isspace(alphabet[i]))
			// Whitespace in alphabet
			return false;

		if (i < 64 && ! isprint(alphabet[i]))
			// Non-printable character in alphabet
			return false;

		if (i == 64 && ! isprint(alphabet[i]) && alphabet[i] != 0x00)
			// Non-printable non-null padding character
			return false;

		const unsigned char och = (unsigned char) alphabet[i];

		if (inverse_alphabet[och] != 0xFF)
			// Duplicated character in alphabet
			return false;

		// Populate character positions
		inverse_alphabet[och] = (i < 64 ? i : 0xFE);
	}

	if (inverse_alphabet[0x00] != 0xFF && inverse_alphabet[0x00] != 0xFE)
		// Duplicated character in alphabet
		return false;

	// A null string terminator is always end-of-input
	inverse_alphabet[0x00] = 0xFE;

	return true;
}

static size_t ATHEME_FATTR_WUR
base64_decode_run(const char *restrict src, void *const restrict out, const size_t out_len,
                  const unsigned char inverse_alphabet[const restrict static 128])
{
	unsigned char *const dst = (unsigned char *) out;
	const size_t dst_len = out_len;

	size_t src_len = strlen(src);
	size_t written = 0;

	while (src_len != 0)
	{
		unsigned char och[4];
		size_t done;

		for (done = 0; done < 4; done++)
		{
			while (isspace((int) src[done]))
			{
				src++;
				src_len--;

				if (src_len == 0 && done >= 2)
					// We have consumed enough input to process below
					break;

				if (src_len == 0 && done == 0)
					// We didn't consume any more input; we're done
					// This is to handle a valid string that ends in whitespace
					return written;

				if (src_len == 0)
					// End of input (premature)
					return BASE64_FAIL;
			}

			och[done] = (unsigned char) src[done];

			if (och[done] >= 0x80U)
				// Invalid input character (out of 7-bit ASCII range)
				return BASE64_FAIL;

			och[done] = inverse_alphabet[och[done]];

			if (och[done] == 0xFEU)
				// End of input (null terminator / padding character), errors will be handled below
				break;

			if (och[done] == 0xFFU)
				// Invalid input character (not in our alphabet)
				return BASE64_FAIL;
		}

		if (done == 0 && (written % 3) == 0)
			// Ran out of input; we're done
			// This is to handle a valid string that ends in whitespace
			return written;

		if (done <= 1)
			// End of input (premature)
			return BASE64_FAIL;

		if (done > 1)
		{
			if (dst != NULL)
			{
				if (written >= dst_len)
					// Insufficient output buffer space remaining
					return BASE64_FAIL;

				dst[written] = (unsigned char) (och[0] << 0x02U);
				dst[written++] |= (unsigned char) (och[1] >> 0x04U);
			}
			else
				written++;
		}

		if (done > 2)
		{
			if (dst != NULL)
			{
				if (written >= dst_len)
					// Insufficient output buffer space remaining
					return BASE64_FAIL;

				dst[written] = (unsigned char) ((och[1] & 0x0FU) << 0x04U);
				dst[written++] |= (unsigned char) (och[2] >> 0x02U);
			}
			else
				written++;
		}

		if (done > 3)
		{
			if (dst != NULL)
			{
				if (written >= dst_len)
					// Insufficient output buffer space remaining
					return BASE64_FAIL;

				dst[written] = (unsigned char) ((och[2] & 0x03U) << 0x06U);
				dst[written++] |= (unsigned char) och[3];
			}
			else
				written++;
		}

		if (done < 4)
			break;

		src += 4;
		src_len -= 4;
	}

	return written;
}

size_t ATHEME_FATTR_WUR
base64_decode(const char *const restrict src, void *const restrict out, const size_t out_len)
{
	return base64_decode_run(src, out, out_len, inverse_alphabet_default);
}

size_t ATHEME_FATTR_WUR
base64_decode_table(const char *const restrict src, void *const restrict out, const size_t out_len,
                    const char alphabet[const restrict static 65])
{
	unsigned char inverse_alphabet[128];

	if (! base64_alphabet_invert(alphabet, inverse_alphabet))
		// Duplicated or invalid character in alphabet
		return BASE64_FAIL;

	return base64_decode_run(src, out, out_len, inverse_alphabet);
}

static size_t ATHEME_FATTR_WUR
base64_encode_run(const void *const restrict in, const size_t in_len, char *const restrict dst, const size_t dst_len,
                  const char alphabet[const restrict static 65])
{
	const unsigned char *src = (const unsigned char *) in;
	size_t src_len = in_len;

	size_t written = 0;

	if (dst != NULL && in_len > 0 && dst_len == 0)
		// Definitely not enough room
		return BASE64_FAIL;

	while (src_len >= 3)
	{
		if (dst != NULL)
		{
			if ((written + 4) > dst_len)
				// Insufficient output buffer space remaining
				return BASE64_FAIL;

			dst[written++] = alphabet[src[0] >> 0x02U];
			dst[written++] = alphabet[((src[0] & 0x03U) << 0x04U) + (src[1] >> 0x04U)];
			dst[written++] = alphabet[((src[1] & 0x0FU) << 0x02U) + (src[2] >> 0x06U)];
			dst[written++] = alphabet[src[2] & 0x3FU];
		}
		else
			written += 4;

		src += 3;
		src_len -= 3;
	}

	if (src_len == 2)
	{
		if (dst != NULL)
		{
			if ((written + 3) > dst_len)
				// Insufficient output buffer space remaining
				return BASE64_FAIL;

			dst[written++] = alphabet[src[0] >> 0x02U];
			dst[written++] = alphabet[((src[0] & 0x03U) << 0x04U) + (src[1] >> 0x04U)];
			dst[written++] = alphabet[(src[1] & 0x0FU) << 0x02U];

			if (alphabet[0x40] != 0x00)
			{
				if ((written + 1) > dst_len)
					// Insufficient output buffer space remaining
					return BASE64_FAIL;

				dst[written++] = alphabet[0x40];
			}
		}
		else
		{
			written += 3;

			if (alphabet[0x40] != 0x00)
				written++;
		}
	}

	if (src_len == 1)
	{
		if (dst != NULL)
		{
			if ((written + 2) > dst_len)
				// Insufficient output buffer space remaining
				return BASE64_FAIL;

			dst[written++] = alphabet[src[0] >> 0x02U];
			dst[written++] = alphabet[(src[0] & 0x03U) << 0x04U];

			if (alphabet[0x40] != 0x00)
			{
				if ((written + 2) > dst_len)
					// Insufficient output buffer space remaining
					return BASE64_FAIL;

				dst[written++] = alphabet[0x40];
				dst[written++] = alphabet[0x40];
			}
		}
		else
		{
			written += 2;

			if (alphabet[0x40] != 0x00)
				written += 2;
		}
	}

	if (dst != NULL)
		// Don't increase the result variable for the NULL terminator -- it reflects string length
		dst[written] = 0x00;

	return written;
}

size_t ATHEME_FATTR_WUR
base64_encode(const void *const restrict in, const size_t in_len, char *const restrict dst, const size_t dst_len)
{
	return base64_encode_run(in, in_len, dst, dst_len, alphabet_default);
}

size_t ATHEME_FATTR_WUR
base64_encode_table(const void *const restrict in, const size_t in_len, char *const restrict dst,
                    const size_t dst_len, const char alphabet[const restrict static 65])
{
	unsigned char inverse_alphabet[128];

	if (! base64_alphabet_invert(alphabet, inverse_alphabet))
		// Duplicated or invalid character in alphabet
		return BASE64_FAIL;

	return base64_encode_run(in, in_len, dst, dst_len, alphabet);
}
