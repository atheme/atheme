/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_global_storage.c: Simple key->value global storage tool.
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

static mowgli_dictionary_t *mowgli_global_storage_dict = NULL;
static mowgli_spinlock_t *mowgli_global_storage_lock = NULL;

void
mowgli_global_storage_init(void)
{
	mowgli_global_storage_dict = mowgli_dictionary_create(strcasecmp);
	mowgli_global_storage_lock = mowgli_spinlock_create();
}

void *
mowgli_global_storage_get(char *name)
{
	void *ret;

	/* name serves as lock token */
	mowgli_spinlock_lock(mowgli_global_storage_lock, name, NULL);
	ret = mowgli_dictionary_retrieve(mowgli_global_storage_dict, name);
	mowgli_spinlock_unlock(mowgli_global_storage_lock, name, NULL);

	return ret;
}

void
mowgli_global_storage_put(char *name, void *value)
{
	mowgli_spinlock_lock(mowgli_global_storage_lock, NULL, name);
	mowgli_dictionary_add(mowgli_global_storage_dict, name, value);
	mowgli_spinlock_unlock(mowgli_global_storage_lock, NULL, name);
}

void
mowgli_global_storage_free(char *name)
{
	mowgli_spinlock_lock(mowgli_global_storage_lock, name, name);
	mowgli_dictionary_delete(mowgli_global_storage_dict, name);
	mowgli_spinlock_unlock(mowgli_global_storage_lock, name, name);
}
