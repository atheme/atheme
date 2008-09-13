/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_iterator.h: Iterators.
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

#ifndef __MOWGLI_ITERATOR_H__
#define __MOWGLI_ITERATOR_H__

typedef struct _mowgli_iterator {
	struct _mowgli_iterator *prev, *next;
	void *data;
} mowgli_iterator_t;

/* The following are macros which can be used with iterators. */
#define MOWGLI_ITER_FOREACH(n, head) for (n = (head); n; n = n->next)
#define MOWGLI_ITER_FOREACH_NEXT(n, head) for (n = (head); n->next; n = n->next)
#define MOWGLI_ITER_FOREACH_PREV(n, tail) for (n = (tail); n; n = n->prev)
#define MOWGLI_ITER_FOREACH_SAFE(n, tn, head) for (n = (head), tn = n ? n->next : NULL; n != NULL; n = tn, tn = n ? n->next : NULL)

#endif
