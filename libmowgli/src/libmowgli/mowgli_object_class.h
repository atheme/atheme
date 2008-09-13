/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object_class.h: Object class and type management,
 *                        cast checking.
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

#ifndef __MOWGLI_OBJECT_CLASS_H__
#define __MOWGLI_OBJECT_CLASS_H__

typedef void (*mowgli_destructor_t)(void *);

typedef struct {
	char *name;
	mowgli_list_t derivitives;
	mowgli_destructor_t destructor;
	mowgli_boolean_t dynamic;
	mowgli_list_t message_handlers;
} mowgli_object_class_t;

extern void mowgli_object_class_init(mowgli_object_class_t *klass, const char *name, mowgli_destructor_t des, mowgli_boolean_t dynamic);
extern int mowgli_object_class_check_cast(mowgli_object_class_t *klass1, mowgli_object_class_t *klass2);
extern void mowgli_object_class_set_derivitive(mowgli_object_class_t *klass, mowgli_object_class_t *parent);
extern void *mowgli_object_class_reinterpret_impl(/* mowgli_object_t */ void *object, mowgli_object_class_t *klass);
extern mowgli_object_class_t *mowgli_object_class_find_by_name(const char *name);
extern void mowgli_object_class_destroy(mowgli_object_class_t *klass);

#define MOWGLI_REINTERPRET_CAST(object, klass) (klass *) mowgli_object_class_reinterpret_impl(object, mowgli_object_class_find_by_name( # klass ));

#define mowgli_forced_cast(from_type, to_type, from, to)\
do {                                                    \
  union cast_union                                      \
  {                                                     \
    to_type   out;                                      \
    from_type in;                                       \
  } u;                                                  \
  typedef int cant_use_union_cast[                      \
    sizeof (from_type) == sizeof (u)                    \
    && sizeof (from_type) == sizeof (to_type) ? 1 : -1];\
  u.in = from;                                          \
  to = u.out;                                           \
} while (0)

#endif
