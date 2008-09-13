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

#ifndef __MOWGLI_ALLOCATION_POLICY_H__
#define __MOWGLI_ALLOCATION_POLICY_H__

typedef void *(*mowgli_allocation_func_t)(int size);
typedef void (*mowgli_deallocation_func_t)(void *ptr);

typedef struct {
	mowgli_object_t parent;
	mowgli_allocation_func_t allocate;
	mowgli_deallocation_func_t deallocate;
} mowgli_allocation_policy_t;

void mowgli_allocation_policy_init(void);
mowgli_allocation_policy_t *mowgli_allocation_policy_create(const char *name,
	mowgli_allocation_func_t allocator, mowgli_deallocation_func_t deallocator);
mowgli_allocation_policy_t *mowgli_allocation_policy_lookup(const char *name);

/* for mowgli_alloc, et. al */
void mowgli_allocator_set_policy(mowgli_allocation_policy_t *policy);
void mowgli_allocator_set_policy_by_name(const char *name);

#endif
