/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_string.h: Immutable string buffers with cheap manipulation.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007 Pippijn van Steenhoven <pippijn -at- one09.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
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

#ifndef __MOWGLI_STRING_H__
#define __MOWGLI_STRING_H__

typedef struct mowgli_string_ {
	char *str;
	size_t pos;
	size_t size;

	void (*reset)(struct mowgli_string_ *self);
	void (*append)(struct mowgli_string_ *self, const char *src, size_t n);
	void (*append_char)(struct mowgli_string_ *self, const char c);
	void (*destroy)(struct mowgli_string_ *self);
} mowgli_string_t;

extern mowgli_string_t *mowgli_string_create(void);
extern void mowgli_string_reset(mowgli_string_t *self);
extern void mowgli_string_destroy(mowgli_string_t *self);
extern void mowgli_string_append(mowgli_string_t *self, const char *src, size_t n);
extern void mowgli_string_append_char(mowgli_string_t *self, const char c);

#endif
