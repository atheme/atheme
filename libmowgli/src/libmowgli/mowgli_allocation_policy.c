/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_allocation_policy.h: Allocation policy management.
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
static mowgli_patricia_t *mowgli_allocation_policy_dict = NULL;

static void _allocation_policy_key_canon(char *str)
{

}

void
mowgli_allocation_policy_init(void)
{
	mowgli_allocation_policy_dict = mowgli_patricia_create(_allocation_policy_key_canon);

	mowgli_object_class_init(&klass, "mowgli.allocation_policy", NULL, FALSE);
}

mowgli_allocation_policy_t *
mowgli_allocation_policy_create(const char *name, mowgli_allocation_func_t allocator,
	mowgli_deallocation_func_t deallocator)
{
	mowgli_allocation_policy_t *policy;

	if (mowgli_allocation_policy_dict == NULL)
		mowgli_allocation_policy_dict = mowgli_patricia_create(_allocation_policy_key_canon);

	if ((policy = mowgli_patricia_retrieve(mowgli_allocation_policy_dict, name)))
		return policy;

	policy = mowgli_alloc(sizeof(mowgli_allocation_policy_t));
	mowgli_object_init_from_class(mowgli_object(policy), name, &klass);

	policy->allocate = allocator;
	policy->deallocate = deallocator;

	return policy;
}

mowgli_allocation_policy_t *
mowgli_allocation_policy_lookup(const char *name)
{
	if (mowgli_allocation_policy_dict == NULL)
		mowgli_allocation_policy_dict = mowgli_patricia_create(_allocation_policy_key_canon);

	return mowgli_patricia_retrieve(mowgli_allocation_policy_dict, name);
}
