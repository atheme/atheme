/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_bitvector.c: Bitvectors.
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

#include "mowgli.h"

static mowgli_object_class_t klass;

/*
 * mowgli_bitvector_init(void)
 *
 * Initialization code for the mowgli.bitvector library.
 *
 * Inputs:
 *       - none
 *
 * Outputs:
 *       - none
 *
 * Side Effects:
 *       - the mowgli_bitvector_t object class is registered.
 */
void mowgli_bitvector_init(void)
{
	mowgli_object_class_init(&klass, "mowgli_bitvector_t", mowgli_free, FALSE);
}

/*
 * mowgli_bitvector_create(int bits)
 *
 * Creates a bitvector.
 *
 * Inputs:
 *       - amount of bits that the bitvector should store
 *
 * Outputs:
 *       - a mowgli.bitvector object
 *
 * Side Effects:
 *       - none
 */
mowgli_bitvector_t *mowgli_bitvector_create(int bits)
{
	mowgli_bitvector_t *bv = (mowgli_bitvector_t *) mowgli_alloc(sizeof(mowgli_bitvector_t));
	mowgli_object_init(mowgli_object(bv), "mowgli_bitvector_t", &klass, NULL);

	bv->bits    = bits;
	bv->divisor = sizeof(int);
	bv->vector  = (unsigned int *) mowgli_alloc_array(bv->divisor, bv->bits / bv->divisor);

	return bv;
}

/*
 * mowgli_bitvector_set(mowgli_bitvector_t *bv, int slot, mowgli_boolean_t val)
 *
 * Sets a bit either ON or OFF in the bitvector.
 *
 * Inputs:
 *       - a mowgli bitvector object
 *       - a slot
 *       - the value for that slot
 *
 * Outputs:
 *       - nothing
 *
 * Side Effects:
 *       - a bit is either set ON or OFF in the bitvector.
 */
void mowgli_bitvector_set(mowgli_bitvector_t *bv, int slot, mowgli_boolean_t val)
{
	int value = 1 << slot;

	switch(val)
	{
		case FALSE:
			bv->vector[bv->bits / bv->divisor] &= ~value;
			break;
		default:
		case TRUE:
			bv->vector[bv->bits / bv->divisor] |= value;
			break;
	}
}

/*
 * mowgli_bitvector_get(mowgli_bitvector_t *bv, int slot)
 *
 * Returns whether the bit in a given slot is ON or OFF.
 *
 * Inputs:
 *       - a mowgli.bitvector object
 *       - a slot to lookup
 *
 * Outputs:
 *       - TRUE if the bit is on
 *       - FALSE otherwise
 *
 * Side Effects:
 *       - none
 */
mowgli_boolean_t mowgli_bitvector_get(mowgli_bitvector_t *bv, int slot)
{
	int mask = 1 << slot;

	return ((bv->vector[bv->bits / bv->divisor] & mask) != 0) ? TRUE : FALSE;
}

/*
 * mowgli_bitvector_combine(mowgli_bitvector_t *bv1, mowgli_bitvector_t *bv2)
 *
 * Combines two bitvectors together.
 *
 * Inputs:
 *       - two bitvectors to be combined
 *
 * Outputs:
 *       - a new bitvector containing the combined result.
 *
 * Side Effects:
 *       - none
 */
mowgli_bitvector_t *mowgli_bitvector_combine(mowgli_bitvector_t *bv1, mowgli_bitvector_t *bv2)
{
	int bits, iter, bs;
	mowgli_bitvector_t *out;

	return_val_if_fail(bv1 != NULL, NULL);
	return_val_if_fail(bv2 != NULL, NULL);

	/* choose the larger bitwidth */
	bits = bv1->bits > bv2->bits ? bv1->bits : bv2->bits;

	/* create the third bitvector. */
	out = mowgli_bitvector_create(bits);

	/* cache the size of the bitvector in memory. */
	bs = out->bits / out->divisor;

	for (iter = 0; iter < bs; iter++)
	{
		out->vector[iter] |= bv1->vector[iter];
		out->vector[iter] |= bv2->vector[iter];
	}

	return out;
}

/*
 * mowgli_bitvector_xor(mowgli_bitvector_t *bv1, mowgli_bitvector_t *bv2)
 *
 * XORs two bitvectors together.
 *
 * Inputs:
 *       - two bitvectors to be XORed
 *
 * Outputs:
 *       - a new bitvector containing the XORed result.
 *
 * Side Effects:
 *       - none
 */
mowgli_bitvector_t *mowgli_bitvector_xor(mowgli_bitvector_t *bv1, mowgli_bitvector_t *bv2)
{
	int bits, iter, bs;
	mowgli_bitvector_t *out;

	return_val_if_fail(bv1 != NULL, NULL);
	return_val_if_fail(bv2 != NULL, NULL);

	/* choose the larger bitwidth */
	bits = bv1->bits > bv2->bits ? bv1->bits : bv2->bits;

	/* create the third bitvector. */
	out = mowgli_bitvector_create(bits);

	/* cache the size of the bitvector in memory. */
	bs = out->bits / out->divisor;

	for (iter = 0; iter < bs; iter++)
	{
		out->vector[iter] = bv1->vector[iter];
		out->vector[iter] &= ~bv2->vector[iter];
	}

	return out;
}

/*
 * mowgli_bitvector_compare(mowgli_bitvector_t *bv1, mowgli_bitvector_t *bv2)
 *
 * Compares two bitvectors to each other.
 *
 * Inputs:
 *       - two bitvectors to be compared
 *
 * Outputs:
 *       - TRUE if the bitvector is equal
 *       - FALSE otherwise
 *
 * Side Effects:
 *       - none
 */
mowgli_boolean_t mowgli_bitvector_compare(mowgli_bitvector_t *bv1, mowgli_bitvector_t *bv2)
{
	int iter, bs;	
	mowgli_boolean_t ret = TRUE;

	/* cache the size of the bitvector in memory. */
	bs = bv1->bits / bv1->divisor;

	for (iter = 0; iter < bs; iter++)
	{
		if (!(bv1->vector[iter] & bv2->vector[iter]))
			ret = FALSE;
	}

	return ret;
}
