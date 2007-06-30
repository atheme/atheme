/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_alloc.c: Safe, portable implementations of malloc, calloc, and free.
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

#include "mowgli.h"

/*
 * mowgli_alloc_array:
 *
 * Allocates an array of data that contains "count" objects,
 * of "size" size.
 *
 * Usually, this wraps calloc().
 *
 * size: size of objects to allocate.
 * count:  amount of objects to allocate.
 */
void * mowgli_alloc_array(size_t size, size_t count)
{
	return calloc(size, count);
}

/*
 * mowgli_alloc:
 *
 * Allocates an object of "size" size.
 * This is the equivilant of calling mowgli_alloc_array(size, 1).
 *
 * size: size of object to allocate.
 */
void * mowgli_alloc(size_t size)
{
	return mowgli_alloc_array(size, 1);
}

/*
 * mowgli_free:
 *
 * Frees an object back to the system memory pool.
 * Wraps free protecting against common mistakes (reports an error instead).
 *
 * ptr: pointer to object to free.
 */
void mowgli_free(void *ptr)
{
	return_if_fail(ptr != NULL);

	free(ptr);
}
