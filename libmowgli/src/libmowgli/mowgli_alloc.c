/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_alloc.c: Safe, portable implementations of malloc, calloc, and free.
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

/*
 * bootstrapped allocators so that we can initialise without blowing up
 */

static void *
_mowgli_bootstrap_alloc(int size)
{
	return calloc(size, 1);
}

static void
_mowgli_bootstrap_free(void *ptr)
{
	if (ptr)
		free(ptr);
}

static mowgli_allocation_policy_t _mowgli_allocator_bootstrap = {
	{ 0 },
	_mowgli_bootstrap_alloc,
	_mowgli_bootstrap_free
};

static mowgli_allocation_policy_t *_mowgli_allocator = &_mowgli_allocator_bootstrap;

/*
 * \brief Allocates an array of data that contains "count" objects,
 * of "size" size.
 *
 * Usually, this wraps calloc().
 *
 * \param size size of objects to allocate.
 * \param count amount of objects to allocate.
 *
 * \return A pointer to a memory buffer.
 */
void *
mowgli_alloc_array(size_t size, size_t count)
{
	return_val_if_fail(_mowgli_allocator != NULL, NULL);

	return _mowgli_allocator->allocate(size * count);
}

/*
 * \brief Allocates an object of "size" size.
 *
 * This is the equivilant of calling mowgli_alloc_array(size, 1).
 *
 * \param size size of object to allocate.
 *
 * \return A pointer to a memory buffer.
 */
void *
mowgli_alloc(size_t size)
{
	return mowgli_alloc_array(size, 1);
}

/*
 * \brief Frees an object back to the system memory pool.
 *
 * Wraps free protecting against common mistakes (reports an error instead).
 *
 * \param ptr pointer to object to free.
 */
void
mowgli_free(void *ptr)
{
	return_if_fail(_mowgli_allocator != NULL);
	return_if_fail(ptr != NULL);

	_mowgli_allocator->deallocate(ptr);
}

/*
 * \brief Sets the mowgli.allocation_policy used by the allocation primitives.
 *
 * \param policy The mowgli_allocation_policy_t object to use.
 */
void
mowgli_allocator_set_policy(mowgli_allocation_policy_t *policy)
{
	return_if_fail(policy != NULL);

	_mowgli_allocator = policy;
}

/*
 * \brief Sets the mowgli.allocation_policy used by the allocation primitives,
 * when given a name.
 *
 * \param name The name of the policy to use.
 */
void
mowgli_allocator_set_policy_by_name(const char *name)
{
	mowgli_allocation_policy_t *policy;

	return_if_fail(name != NULL);

	policy = mowgli_allocation_policy_lookup(name);

	if (policy == NULL)
		return;

	mowgli_allocator_set_policy(policy);
}
