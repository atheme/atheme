/*
 * $Id$
 *
 * Copyright Metaparadigm Pte. Ltd. 2004.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public (LGPL)
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details: http://www.gnu.org/
 *
 */

#ifndef _printbuf_h_
#define _printbuf_h_

struct printbuf
{
    char *buf;
    int bpos;
    int size;
};

extern struct printbuf *printbuf_new();

extern int printbuf_memappend(struct printbuf *p, char *buf, int size);

extern int sprintbuf(struct printbuf *p, const char *msg, ...);

extern void printbuf_reset(struct printbuf *p);

extern void printbuf_free(struct printbuf *p);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
