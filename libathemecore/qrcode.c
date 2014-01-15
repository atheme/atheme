/*
 * atheme-services: A collection of minimalist IRC services
 * qrcode.c: IRC encoding of QR codes
 *
 * Copyright (c) 2014 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

#ifdef HAVE_LIBQRENCODE
# include <qrencode.h>
#endif

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
command_success_qrcode(sourceinfo_t *si, const char *data)
{
#ifdef HAVE_LIBQRENCODE
	char *buf;
	QRcode *code;
	size_t bufsize, realwidth;
	size_t y;

	return_if_fail(si != NULL);
	return_if_fail(data != NULL);

	code = QRcode_encodeData(strlen(data), data, 4, QR_ECLEVEL_L);

	realwidth = (code->width + 3 * 2) * 2;
	bufsize = strlen(prologue) + (realwidth * 3) + strlen(prologue);
	buf = smalloc(bufsize);

	/* header */
	for (y = 0; y < 3; y++)
	{
		qrcode_margin(buf, bufsize, realwidth);
		command_success_nodata(si, "%s", buf);
	}

	/* qrcode contents + side margins */
	for (y = 0; y < code->width; y++)
	{
		qrcode_scanline(buf, bufsize, code->data + (y * code->width), code->width);
		command_success_nodata(si, "%s", buf);
	}

	/* footer */
	for (y = 0; y < 3; y++)
	{
		qrcode_margin(buf, bufsize, realwidth);
		command_success_nodata(si, "%s", buf);
	}

	free(buf);
	QRcode_free(code);
#endif
}
