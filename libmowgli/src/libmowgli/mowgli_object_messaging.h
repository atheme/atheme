/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object_messaging.h: Object event notification and message passing.
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
