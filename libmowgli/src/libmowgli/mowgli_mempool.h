/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_mempool.h: Memory pooling.
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

#ifndef __MOWGLI_MEMPOOL_H__
#define __MOWGLI_MEMPOOL_H__

typedef struct mowgli_mempool_t_ mowgli_mempool_t;

mowgli_mempool_t * mowgli_mempool_create(void);
mowgli_mempool_t * mowgli_mempool_with_custom_destructor(mowgli_destructor_t destructor);

void * mowgli_mempool_add(mowgli_mempool_t * pool, void * ptr);
void * mowgli_mempool_allocate(mowgli_mempool_t * pool, size_t sz);
void mowgli_mempool_release(mowgli_mempool_t * pool, void * addr);

void mowgli_mempool_cleanup(mowgli_mempool_t * pool);

void mowgli_mempool_destroy(mowgli_mempool_t * pool);

char * mowgli_mempool_strdup(mowgli_mempool_t * pool, char * src);

#define mowgli_mempool_alloc_object(pool, obj) \
	mowgli_mempool_allocate(pool, sizeof(obj))

#endif
