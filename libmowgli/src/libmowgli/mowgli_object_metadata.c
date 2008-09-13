/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object_metadata.c: Object metadata.
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

void mowgli_object_metadata_associate(mowgli_object_t *self, const char *key, void *value)
{
	mowgli_object_metadata_entry_t *e = NULL;
	mowgli_node_t *n;

	if (self == NULL)
		mowgli_throw_exception(mowgli.object_metadata.invalid_object_exception);

	if (key == NULL)
		mowgli_throw_exception(mowgli.null_pointer_exception);

	MOWGLI_LIST_FOREACH(n, self->metadata.head)
	{
		e = (mowgli_object_metadata_entry_t *) n->data;

		if (!strcasecmp(e->name, key))
			break;
	}

	if (e != NULL)
	{
		e->data = value;
		return;
	}

	e = mowgli_alloc(sizeof(mowgli_object_metadata_entry_t));
	e->name = strdup(key);
	e->data = value;

	mowgli_node_add(e, mowgli_node_create(), &self->metadata);
}

void mowgli_object_metadata_dissociate(mowgli_object_t *self, const char *key)
{
	mowgli_object_metadata_entry_t *e;
	mowgli_node_t *n, *tn;

	if (self == NULL)
		mowgli_throw_exception(mowgli.object_metadata.invalid_object_exception);

	if (key == NULL)
		mowgli_throw_exception(mowgli.null_pointer_exception);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, self->metadata.head)
	{
		e = (mowgli_object_metadata_entry_t *) n->data;

		if (!strcasecmp(e->name, key))
		{
			mowgli_node_delete(n, &self->metadata);
			mowgli_node_free(n);

			mowgli_free(e->name);
			mowgli_free(e);
		}
	}
}

void *mowgli_object_metadata_retrieve(mowgli_object_t *self, const char *key)
{
	mowgli_object_metadata_entry_t *e;
	mowgli_node_t *n;

	if (self == NULL)
		mowgli_throw_exception_val(mowgli.object_metadata.invalid_object_exception, NULL);

	if (key == NULL)
		mowgli_throw_exception_val(mowgli.null_pointer_exception, NULL);

	MOWGLI_LIST_FOREACH(n, self->metadata.head)
	{
		e = (mowgli_object_metadata_entry_t *) n->data;

		if (!strcasecmp(e->name, key))
			return e->data;
	}

	return NULL;
}
