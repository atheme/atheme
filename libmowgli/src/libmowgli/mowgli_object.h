/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object.h: Object management.
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

#ifndef __MOWGLI_OBJECT_H__
#define __MOWGLI_OBJECT_H__

typedef struct {
	char *name;
	int refcount;
	mowgli_object_class_t *klass;
	mowgli_list_t message_handlers;
	mowgli_list_t metadata;
} mowgli_object_t;

extern void mowgli_object_init(mowgli_object_t *, const char *name, mowgli_object_class_t *klass, mowgli_destructor_t destructor);
extern void mowgli_object_init_from_class(mowgli_object_t *, const char *, mowgli_object_class_t *klass);
extern void *mowgli_object_ref(void *);
extern void mowgli_object_unref(void *);

#define mowgli_object(x) ((mowgli_object_t *) x)

#endif
