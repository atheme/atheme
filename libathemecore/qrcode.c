/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2014 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * qrcode.c: IRC encoding of QR codes
 */

#include <atheme.h>
#include "internal.h"

#include <qrencode.h>

static const char prologue[] = "\0031,15";
static const char invert = 22;
static const char reset = 15;
static const size_t prologuelen = 5;

static void
qrcode_margin(char *buffer, size_t bufsize, size_t width)
{
	memset(buffer, 0, bufsize);

	memcpy(buffer, prologue, prologuelen);
	memset(buffer + prologuelen, ' ', width);
}

static void
qrcode_scanline(char *buffer, size_t bufsize, unsigned char *row, size_t width)
{
	size_t x;
	char last;
	char *p = buffer;

	memset(buffer, 0, bufsize);

	memcpy(p, prologue, prologuelen);
	p += prologuelen;

	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';

	last = 0;

	for (x = 0; x < width; x++)
	{
		if ((row[x] & 0x1) != (last & 0x1))
			*p++ = invert;

		*p++ = ' ';
		*p++ = ' ';

		last = row[x];
	}

	*p++ = reset;

	memcpy(p, prologue, prologuelen);
	p += prologuelen;

	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
	*p++ = ' ';
}

void
command_success_qrcode(struct sourceinfo *si, const char *data)
{
	char *buf;
	QRcode *code;
	size_t bufsize, realwidth;

	return_if_fail(si != NULL);
	return_if_fail(data != NULL);

	code = QRcode_encodeData(strlen(data), (const void *) data, 4, QR_ECLEVEL_L);

	realwidth = (code->width + 3 * 2) * 2;
	bufsize = strlen(prologue) + (realwidth * 3) + strlen(prologue);
	buf = smalloc(bufsize);

	/* header */
	for (int y = 0; y < 3; y++)
	{
		qrcode_margin(buf, bufsize, realwidth);
		command_success_nodata(si, "%s", buf);
	}

	/* qrcode contents + side margins */
	for (int y = 0; y < code->width; y++)
	{
		qrcode_scanline(buf, bufsize, code->data + (y * code->width), (size_t) code->width);
		command_success_nodata(si, "%s", buf);
	}

	/* footer */
	for (int y = 0; y < 3; y++)
	{
		qrcode_margin(buf, bufsize, realwidth);
		command_success_nodata(si, "%s", buf);
	}

	sfree(buf);
	QRcode_free(code);
}
