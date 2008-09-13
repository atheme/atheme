/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_random.c: Portable mersinne-twister based psuedo-random number generator.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Algorithm copyright (c) 1999-2007 Takuji Nishimura and Makoto Matsumoto
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

#include "mowgli.h"

/* period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

/* mowgli_random_t contains state data which is private */
struct mowgli_random_
{
	mowgli_object_t object;
	unsigned int mt[N];
	size_t mti;
};

static mowgli_object_class_t klass;

/* initialization */
void mowgli_random_init(void)
{
	mowgli_object_class_init(&klass, "mowgli_random_t", NULL, FALSE);
}

/* construction and destruction. */
mowgli_random_t *mowgli_random_create(void)
{
	return mowgli_random_create_with_seed(time(NULL));
}

mowgli_random_t *mowgli_random_create_with_seed(unsigned int seed)
{
	mowgli_random_t *out = mowgli_alloc(sizeof(mowgli_random_t));
	mowgli_object_init(mowgli_object(out), NULL, &klass, NULL);

	mowgli_random_reseed(out, seed);

	return out;
}

/* reset seed */
void mowgli_random_reseed(mowgli_random_t *self, unsigned int seed)
{
	return_if_fail(self != NULL);

	self->mt[0] = seed & 0xffffffffUL;
	for (self->mti = 1; self->mti < N; self->mti++)
	{
		self->mt[self->mti] = (1812433253UL * (self->mt[self->mti - 1] ^ (self->mt[self->mti - 1] >> 30)) + self->mti);
		self->mt[self->mti] &= 0xffffffffUL;
	}
}

/* number retrieval */
unsigned int mowgli_random_int(mowgli_random_t *self)
{
	unsigned int y;
	static unsigned int mag01[2] = { 0x0UL, MATRIX_A };

	return_val_if_fail(self != NULL, 0);

	if (self->mti >= N)
	{
		int t;

		for (t = 0; t < N - M; t++)
		{
			y = (self->mt[t] & UPPER_MASK) | (self->mt[t + 1] & LOWER_MASK);
			self->mt[t] = self->mt[t + M] ^ (y >> 1) ^ mag01[y & 0x1U];
		}

		for (; t < N - 1; t++)
		{
			y = (self->mt[t] & UPPER_MASK) | (self->mt[t + 1] & LOWER_MASK);
			self->mt[t] = self->mt[t + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1U];			
		}

		y = (self->mt[N - 1] & UPPER_MASK) | (self->mt[0] & LOWER_MASK);
		self->mt[N - 1] = self->mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1U];
		self->mti = 0;
	}

	y = self->mt[self->mti++];

	/* tempering */
	y ^= (y >> 11);
	y ^= (y >> 7) & 0x9d2c5680U;
	y ^= (y >> 15) & 0xefc60000U;
	y ^= (y >> 18);

	return y;
}

int mowgli_random_int_ranged(mowgli_random_t *self, int begin, int end)
{
	int dist = end - begin;
	unsigned int max, ret;

	if (dist <= 0x80000000U)
	{
		unsigned int remain = (0x80000000U % dist) * 2;

		if (remain >= dist)
			remain -= dist;

		max = 0xFFFFFFFFU - remain;
	} else
		max = dist - 1;

	do
	{
		ret = mowgli_random_int(self);
	} while (ret > max);

	ret %= dist;

	return begin + ret;
}
