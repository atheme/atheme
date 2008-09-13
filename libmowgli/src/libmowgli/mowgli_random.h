/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_random.h: Portable mersinne-twister based psuedo-random number generator.
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

#ifndef __MOWGLI_RANDOM_H__
#define __MOWGLI_RANDOM_H__

/* mowgli_random_t contains state data which is private */
struct mowgli_random_;
typedef struct mowgli_random_ mowgli_random_t;

/* object class initialization. */
extern void mowgli_random_init(void);

/* construction and destruction. */
extern mowgli_random_t *mowgli_random_create(void);
extern mowgli_random_t *mowgli_random_create_with_seed(unsigned int seed);

/* reset seed */
extern void mowgli_random_reseed(mowgli_random_t *self, unsigned int seed);

/* number retrieval */
extern unsigned int mowgli_random_int(mowgli_random_t *self);
extern int mowgli_random_int_ranged(mowgli_random_t *self, int begin, int end);

#endif
