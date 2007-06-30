/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_error_backtrace.h: Print errors and explain how they were reached.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef __MOWGLI_ERROR_BACKTRACE_H__
#define __MOWGLI_ERROR_BACKTRACE_H__

typedef struct mowgli_error_context_ {
	mowgli_list_t bt;
} mowgli_error_context_t;

extern void mowgli_error_context_display(mowgli_error_context_t *e, const char *delim);
extern void mowgli_error_context_display_with_error(mowgli_error_context_t *e, const char *delim, const char *error);
extern void mowgli_error_context_destroy(mowgli_error_context_t *e);
extern void mowgli_error_context_push(mowgli_error_context_t *e, const char *msg, ...);
extern void mowgli_error_context_pop(mowgli_error_context_t *e);
extern mowgli_error_context_t *mowgli_error_context_create(void);

#endif
