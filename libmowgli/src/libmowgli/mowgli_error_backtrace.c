/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_error_backtrace.c: Print errors and explain how they were reached.
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

void
mowgli_error_context_display(mowgli_error_context_t *e, const char *delim)
{
	mowgli_node_t *n;
	char *bt_msg;

	return_if_fail(e != NULL);
	return_if_fail(delim != NULL);

	if (MOWGLI_LIST_LENGTH(&e->bt) == 0)
		mowgli_throw_exception(mowgli.error_backtrace.no_backtrace);

	MOWGLI_LIST_FOREACH(n, e->bt.head)
	{
		bt_msg = (char *) n->data;

		printf("%s%s", bt_msg, n->next != NULL ? delim : "\n");
	}
}

void
mowgli_error_context_destroy(mowgli_error_context_t *e)
{
	mowgli_node_t *n, *tn;

	return_if_fail(e != NULL);

	if (MOWGLI_LIST_LENGTH(&e->bt) == 0)
	{
		mowgli_free(e);
		return;
	}

	MOWGLI_LIST_FOREACH_SAFE(n, tn, e->bt.head)
	{
		mowgli_free(n->data);

		mowgli_node_delete(n, &e->bt);
		mowgli_node_free(n);
	}

	mowgli_free(e);
}

void
mowgli_error_context_display_with_error(mowgli_error_context_t *e, const char *delim, const char *error)
{
	mowgli_error_context_display(e, delim);
	printf("Error: %s\n", error);

	exit(EXIT_FAILURE);
}

void
mowgli_error_context_push(mowgli_error_context_t *e, const char *msg, ...)
{
	char buf[65535];
	va_list va;

	return_if_fail(e != NULL);
	return_if_fail(msg != NULL);

	va_start(va, msg);
	vsnprintf(buf, 65535, msg, va);
	va_end(va);

	mowgli_node_add(strdup(buf), mowgli_node_create(), &e->bt);
}

void
mowgli_error_context_pop(mowgli_error_context_t *e)
{
	return_if_fail(e != NULL);

	mowgli_node_delete(e->bt.tail, &e->bt);
}

mowgli_error_context_t *
mowgli_error_context_create(void)
{
	mowgli_error_context_t *out = (mowgli_error_context_t *) mowgli_alloc(sizeof(mowgli_error_context_t));

	return out;
}
