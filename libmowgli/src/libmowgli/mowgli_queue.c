/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_queue.c: Double-ended queues, also known as deque.
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

static mowgli_heap_t *mowgli_queue_heap = NULL;

void
mowgli_queue_init(void)
{
	mowgli_queue_heap = mowgli_heap_create(sizeof(mowgli_queue_t), 256, BH_NOW);

	if (mowgli_queue_heap == NULL)
		mowgli_log("mowgli_queue_heap was not created, expect problems.");
}

mowgli_queue_t *
mowgli_queue_shift(mowgli_queue_t *head, void *data)
{
	mowgli_queue_t *out = mowgli_heap_alloc(mowgli_queue_heap);

	out->next = head;
	out->data = data;

	if (head != NULL)
	{
		out->prev = head->prev;

		if (out->prev != NULL)
			out->prev->next = out;

		head->prev = out;
	}

	return out;
}

mowgli_queue_t *
mowgli_queue_push(mowgli_queue_t *head, void *data)
{
	mowgli_queue_t *out = mowgli_heap_alloc(mowgli_queue_heap);

	out->prev = head;
	out->data = data;

	if (head != NULL)
		head->next = out;

	return out;
}

mowgli_queue_t *
mowgli_queue_remove(mowgli_queue_t *head)
{
	mowgli_queue_t *out;

	if (head->prev != NULL)
		head->prev->next = head->next;

	if (head->next != NULL)
		head->next->prev = head->prev;

	out = head->prev != NULL ? head->prev : head->next;

	mowgli_heap_free(mowgli_queue_heap, head);

	return out;
}

mowgli_queue_t *
mowgli_queue_find(mowgli_queue_t *head, void *data)
{
	mowgli_queue_t *n;

	for (n = head; n != NULL; n = n->next)
		if (n->data == data)
			return n;

	return NULL;
}

mowgli_queue_t *
mowgli_queue_remove_data(mowgli_queue_t *head, void *data)
{
	mowgli_queue_t *n = mowgli_queue_find(head, data);

	if (n != NULL)
		return mowgli_queue_remove(n);

	return NULL;
}

void
mowgli_queue_destroy(mowgli_queue_t *head)
{
	mowgli_queue_t *n, *n2;

	for (n = head, n2 = n ? n->next : NULL; n != NULL; n = n2, n2 = n ? n->next : NULL)
		mowgli_queue_remove(n);
}

mowgli_queue_t *
mowgli_queue_skip(mowgli_queue_t *head, int nodes)
{
	mowgli_queue_t *n;
	int iter;

	for (iter = 0, n = head; n != NULL && iter < nodes; n = n->next, iter++);

	return n;
}

mowgli_queue_t *
mowgli_queue_rewind(mowgli_queue_t *head, int nodes)
{
	mowgli_queue_t *n;
	int iter;

	for (iter = 0, n = head; n != NULL && iter < nodes; n = n->prev, iter++);

	return n;
}

mowgli_queue_t *
mowgli_queue_head(mowgli_queue_t *n)
{
	mowgli_queue_t *tn;

	for (tn = n; tn != NULL && tn->prev != NULL; tn = tn->prev);

	return tn;
}

mowgli_queue_t *
mowgli_queue_tail(mowgli_queue_t *n)
{
	mowgli_queue_t *tn;

	for (tn = n; tn != NULL && tn->next != NULL; tn = tn->next);

	return tn;
}

void *
mowgli_queue_pop_head(mowgli_queue_t **n)
{
	mowgli_queue_t *tn;
	void *out;

	return_val_if_fail(n != NULL, NULL);
	return_val_if_fail(*n != NULL, NULL);

	tn = *n;
	out = tn->data;
	*n = tn->next;

	mowgli_queue_remove(tn);

	return out;
}

void *
mowgli_queue_pop_tail(mowgli_queue_t **n)
{
	mowgli_queue_t *tn;
	void *out;

	return_val_if_fail(n != NULL, NULL);
	return_val_if_fail(*n != NULL, NULL);

	tn = *n;
	out = tn->data;
	*n = tn->prev;

	mowgli_queue_remove(tn);

	return out;
}

int
mowgli_queue_length(mowgli_queue_t *head)
{
	int iter;
	mowgli_queue_t *n;

	for (n = head, iter = 0; n != NULL; n = n->next, iter++);

	return iter;
}
