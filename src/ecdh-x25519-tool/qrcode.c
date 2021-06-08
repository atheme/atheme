/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Helper utility for the SASL ECDH-X25519-CHALLENGE mechanism.
 */

#include <atheme.h>

#include "ecdh-x25519-challenge.h"
#include "ecdh-x25519-tool.h"

#ifdef HAVE_LIBQRENCODE

#include <qrencode.h>

static void
ecdh_x25519_tool_print_qrcode_stdout(QRcode *const restrict qrcode)
{
	const char empty[] = " ";
	const char beginline[] = "\n  ";
	const char lowhalf[] = "\342\226\204";
	const char tophalf[] = "\342\226\200";
	const char full[] = "\342\226\210";

	const size_t width = (size_t) qrcode->width;
	const size_t realwidth = width + 2;

	(void) fputs(beginline, stdout);

	for (size_t x = 0; x < realwidth; x++)
		(void) fputs(lowhalf, stdout);

	for (size_t x = 0; x < width; x += 2)
	{
		unsigned char *row1 = qrcode->data + (width * x);
		unsigned char *row2 = row1 + width;

		(void) fputs(beginline, stdout);
		(void) fputs(full, stdout);

		for (size_t y = 0; y < width; y++)
		{
			if (row1[y] & 0x01U)
			{
				if (x < (width - 1) && row2[y] & 0x01U)
					(void) fputs(empty, stdout);
				else
					(void) fputs(lowhalf, stdout);
			}
			else if (x < (width - 1) && row2[y] & 0x01U)
				(void) fputs(tophalf, stdout);
			else
				(void) fputs(full, stdout);
		}

		(void) fputs(full, stdout);
	}

	(void) fputs("\n", stdout);
}

int
ecdh_x25519_tool_print_qrcode(const char *const restrict keyfile_path)
{
	unsigned char secpubkey[ATHEME_ECDH_X25519_XKEY_LEN * 2U];

	FILE *const fh = ((keyfile_path != NULL) ? fopen(keyfile_path, "rb") : stdin);

	if (! fh)
	{
		(void) print_stderr("fopen('%s', 'rb'): %s", keyfile_path, strerror(errno));
		return EXIT_FAILURE;
	}
	if (fread(secpubkey, sizeof secpubkey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read private and public keypair from key file"));
		return EXIT_FAILURE;
	}

	(void) fclose(fh);

	QRcode *const qrcode = QRcode_encodeData((int) sizeof secpubkey, secpubkey, 0, QR_ECLEVEL_L);

	if (! qrcode)
	{
		(void) print_stderr(_("Unable to encode keypair: %s"), strerror(errno));
		return EXIT_FAILURE;
	}

	(void) ecdh_x25519_tool_print_qrcode_stdout(qrcode);
	return EXIT_SUCCESS;
}

#else /* HAVE_LIBQRENCODE */

int
ecdh_x25519_tool_print_qrcode(const char ATHEME_VATTR_UNUSED *const restrict keyfile_path)
{
	(void) print_stderr(_("This program was not built with QR-Code support!"));
	return EXIT_FAILURE;
}

#endif /* !HAVE_LIBQRENCODE */
