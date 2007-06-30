/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object_messaging.h: Object event notification and message passing.
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

#ifndef __MOWGLI_OBJECT_MESSAGING_H__
#define __MOWGLI_OBJECT_MESSAGING_H__

typedef struct mowgli_object_message_handler_ mowgli_object_message_handler_t;
typedef void (*mowgli_object_messaging_func_t)(mowgli_object_t *self, mowgli_object_message_handler_t *sig, mowgli_argstack_t *argstack);

struct mowgli_object_message_handler_ {
	char *name;
	char *descstr;
	mowgli_object_messaging_func_t handler;
};

extern void mowgli_object_class_message_handler_attach(mowgli_object_class_t *klass, mowgli_object_message_handler_t *sig);
extern void mowgli_object_class_message_handler_detach(mowgli_object_class_t *klass, mowgli_object_message_handler_t *sig);
extern void mowgli_object_message_handler_attach(mowgli_object_t *self, mowgli_object_message_handler_t *sig);
extern void mowgli_object_message_handler_detach(mowgli_object_t *self, mowgli_object_message_handler_t *sig);
extern void mowgli_object_message_broadcast(mowgli_object_t *self, const char *name, ...);

#endif
