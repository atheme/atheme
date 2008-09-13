/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_argstack.h: Argument stacks.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
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

#ifndef __MOWGLI_ARGSTACK_H__
#define __MOWGLI_ARGSTACK_H__

typedef enum {
	MOWGLI_ARG_NUMERIC,
	MOWGLI_ARG_POINTER,
	MOWGLI_ARG_STRING,
	MOWGLI_ARG_BOOLEAN
} mowgli_argstack_element_type_t;

typedef struct {
	union {
		int numeric;
		void *pointer;
		char *string;
		mowgli_boolean_t boolean;
	} data;
	mowgli_argstack_element_type_t type;
} mowgli_argstack_element_t;

typedef struct {
	mowgli_object_t parent;
	mowgli_list_t stack;
} mowgli_argstack_t;

extern mowgli_argstack_t *mowgli_argstack_create(const char *descstr, ...);
extern mowgli_argstack_t *mowgli_argstack_create_from_va_list(const char *descstr, va_list va);
extern void mowgli_argstack_init(void);
extern const char *mowgli_argstack_pop_string(mowgli_argstack_t *);
extern int mowgli_argstack_pop_numeric(mowgli_argstack_t *);
extern mowgli_boolean_t mowgli_argstack_pop_boolean(mowgli_argstack_t *);
extern void *mowgli_argstack_pop_pointer(mowgli_argstack_t *);

#endif
