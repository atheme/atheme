/*
 * atheme-services: A collection of minimalist IRC services
 * ubase64.c: ircu base64 routines.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
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

/*
 * base64touint() written 01/20/06 by jilles, for getting IP addresses.
 */

static const char ub64_alphabet[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";
#define __ '\377'
static const char ub64_lookuptab[256] = {
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 0-15 */
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 16-31 */
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 32-47 */
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, __, __, __, __, __, __, /* 48-63 */
__, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, /* 64-79 */
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 62, __, 63, __, __, /* 80-95 */
__, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 96-111 */
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, __, __, __, __, __, /* 112-127 */
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,
__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ };
#undef __

const char *uinttobase64(char *buf, uint64_t v, int64_t count)
{
	buf[count] = '\0';

	while (count >= 0)
	{
		buf[--count] = ub64_alphabet[v & 63];
		v >>= 6;
	}

	return buf;
}

unsigned int base64touint(char *buf)
{
	int bits;
	unsigned int v = 0;
	int count = 6;

	while (--count >= 0 && (bits = ub64_lookuptab[255 & *buf++]) != '\377')
		v = v << 6 | bits;
	return v;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
