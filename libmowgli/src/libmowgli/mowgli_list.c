/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_list.c: Linked lists.
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

static mowgli_heap_t *mowgli_node_heap;
static mowgli_heap_t *mowgli_list_heap;

void mowgli_node_init(void)
{
        mowgli_node_heap = mowgli_heap_create(sizeof(mowgli_node_t), 1024, BH_NOW);
        mowgli_list_heap = mowgli_heap_create(sizeof(mowgli_list_t), 64, BH_NOW);

	if (mowgli_node_heap == NULL || mowgli_list_heap == NULL)
	{
		mowgli_log("heap allocator failure.");
		abort();
	}
}

/* creates a new node */
mowgli_node_t *mowgli_node_create(void)
{
        mowgli_node_t *n;

        /* allocate it */
        n = mowgli_heap_alloc(mowgli_node_heap);

        /* initialize */
        n->next = n->prev = n->data = NULL;

        /* return a pointer to the new node */
        return n;
}

/* frees a node */
void mowgli_node_free(mowgli_node_t *n)
{
	return_if_fail(n != NULL);

        /* free it */
        mowgli_heap_free(mowgli_node_heap, n);
}

/* adds a node to the end of a list */
void mowgli_node_add(void *data, mowgli_node_t *n, mowgli_list_t *l)
{
	mowgli_node_t *tn;

	return_if_fail(n != NULL);
	return_if_fail(l != NULL);

	n->next = n->prev = NULL;
	n->data = data;

	/* first node? */
	if (l->head == NULL)
	{
		l->head = n;
		l->tail = n;
		l->count++;
		return;
	}

	/* use the cached tail. */
	tn = l->tail;

	/* set the our `prev' to the last node */
	n->prev = tn;

	/* set the last node's `next' to us */
	n->prev->next = n;

	/* set the list's `tail' to us */
	l->tail = n;

	/* up the count */
	l->count++;
}

/* adds a node to the head of a list */
void mowgli_node_add_head(void *data, mowgli_node_t *n, mowgli_list_t *l)
{
	mowgli_node_t *tn;

	return_if_fail(n != NULL);
	return_if_fail(l != NULL);

	n->next = n->prev = NULL;
	n->data = data;

	/* first node? */
	if (!l->head)
	{
		l->head = n;
		l->tail = n;
		l->count++;
		return;
	}

	tn = l->head;
	n->next = tn;
	tn->prev = n;
	l->head = n;
	l->count++;
}

/* adds a node to a list before another node, or to the end */
void mowgli_node_add_before(void *data, mowgli_node_t *n, mowgli_list_t *l, mowgli_node_t *before)
{
	return_if_fail(n != NULL);
	return_if_fail(l != NULL);

	if (before == NULL)
		mowgli_node_add(data, n, l);
	else if (before == l->head)
		mowgli_node_add_head(data, n, l);
	else
	{
		n->data = data;
		n->prev = before->prev;
		n->next = before;
		before->prev = n;
		n->prev->next = n;
		l->count++;
	}
}

/* adds a node to a list after another node, or to the end */
void mowgli_node_add_after(void *data, mowgli_node_t *n, mowgli_list_t *l, mowgli_node_t *before)
{
	return_if_fail(n != NULL);
	return_if_fail(l != NULL);

	if (before == NULL || before->next == NULL)
		mowgli_node_add(data, n, l);
	else
	{
		n->data = data;
		n->prev = before;
		n->next = before->next;
		before->next = n;
		n->next->prev = n;
		l->count++;
	}
}

/* retrieves a node at `position` position. */
mowgli_node_t *mowgli_node_nth(mowgli_list_t *l, int pos)
{
	int iter;
	mowgli_node_t *n;

	return_val_if_fail(l != NULL, NULL);

	if (pos < 0)
		return NULL;

	/* locate the proper position. */
	if (pos < MOWGLI_LIST_LENGTH(l) / 2)
		for (iter = 0, n = l->head; iter != pos && n != NULL; iter++, n = n->next);
	else
		for (iter = MOWGLI_LIST_LENGTH(l), n = l->tail;
			iter != pos && n != NULL; iter--, n = n->prev);

	return n;
}

/* returns the data from node at `position` position, or NULL. */
void *mowgli_node_nth_data(mowgli_list_t *l, int pos)
{
	mowgli_node_t *n;

	return_val_if_fail(l != NULL, NULL);

	n = mowgli_node_nth(l, pos);

	if (n == NULL)
		return NULL;

	return n->data;
}

/* inserts a node at `position` position. */
void mowgli_node_insert(void *data, mowgli_node_t *n, mowgli_list_t *l, int pos)
{
	mowgli_node_t *tn;

	return_if_fail(n != NULL);
	return_if_fail(l != NULL);

	/* locate the proper position. */
	tn = mowgli_node_nth(l, pos);

	mowgli_node_add_before(data, n, l, tn);
}

/* retrieves the index position of a node in a list. */
int mowgli_node_index(mowgli_node_t *n, mowgli_list_t *l)
{
	int iter;
	mowgli_node_t *tn;

	return_val_if_fail(n != NULL, -1);
	return_val_if_fail(l != NULL, -1);

	/* locate the proper position. */
	for (iter = 0, tn = l->head; tn != n && tn != NULL; iter++, tn = tn->next);

	return iter < MOWGLI_LIST_LENGTH(l) ? iter : -1;
}

/* deletes a link between a node and a list. */
void mowgli_node_delete(mowgli_node_t *n, mowgli_list_t *l)
{
	return_if_fail(n != NULL);
	return_if_fail(l != NULL);

        /* are we the head? */
        if (!n->prev)
                l->head = n->next;
        else
                n->prev->next = n->next;

        /* are we the tail? */
        if (!n->next)
                l->tail = n->prev;
        else
                n->next->prev = n->prev;

        /* down the count */
        l->count--;
}

/* finds a node by `data' */
mowgli_node_t *mowgli_node_find(void *data, mowgli_list_t *l)
{
	mowgli_node_t *n;

	return_val_if_fail(l != NULL, NULL);

	MOWGLI_LIST_FOREACH(n, l->head) if (n->data == data)
		return n;

	return NULL;
}

/* moves a node from one list to another. */
void mowgli_node_move(mowgli_node_t *m, mowgli_list_t *oldlist, mowgli_list_t *newlist)
{
	return_if_fail(m != NULL);
	return_if_fail(oldlist != NULL);
	return_if_fail(newlist != NULL);

        /* Assumption: If m->next == NULL, then list->tail == m
         *        and: If m->prev == NULL, then list->head == m
         */
        if (m->next != NULL)
                m->next->prev = m->prev;
        else
                oldlist->tail = m->prev;

        if (m->prev != NULL)
                m->prev->next = m->next;
        else
                oldlist->head = m->next;

        m->prev = NULL;
        m->next = newlist->head;

        if (newlist->head != NULL)
                newlist->head->prev = m;
        else if (newlist->tail == NULL)
                newlist->tail = m;

        newlist->head = m;

        oldlist->count--;
        newlist->count++;
}

/* creates a new list. */
mowgli_list_t *mowgli_list_create(void)
{
	mowgli_list_t *out = mowgli_heap_alloc(mowgli_list_heap);

	return out;
}

/* frees a created list. */
void mowgli_list_free(mowgli_list_t *l)
{
	mowgli_heap_free(mowgli_list_heap, l);
}

/* concatenates two lists together. */
void mowgli_list_concat(mowgli_list_t *l, mowgli_list_t *l2)
{
	return_if_fail(l != NULL);
	return_if_fail(l2 != NULL);

	l->tail->next = l2->head;
	l->tail->next->prev = l->tail;
	l->tail = l2->tail;
	l->count += l2->count;

	/* clear out l2 as it is no longer needed. */
	l2->head = l2->tail = NULL;
	l2->count = 0;
}

/* reverse a list -- O(n)! */
void mowgli_list_reverse(mowgli_list_t *l)
{
	mowgli_node_t *n, *tn;

	return_if_fail(l != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, l->head)
	{
		mowgli_node_t *tn2 = n->next;
		n->next = n->prev;
		n->prev = tn2;
	}

	tn = l->head;
	l->head = l->tail;
	l->tail = tn;
}

/* sorts a list -- O(n ^ 2) most likely, i don't want to think about it. --nenolod */
void mowgli_list_sort(mowgli_list_t *l, mowgli_list_comparator_t comp, void *opaque)
{
	mowgli_node_t *n, *tn, *n2, *tn2;

	return_if_fail(l != NULL);
	return_if_fail(comp != NULL);

	MOWGLI_LIST_FOREACH_SAFE(n, tn, l->head)
	{
		MOWGLI_LIST_FOREACH_SAFE(n2, tn2, l->head)
		{
			int result;
			int i, i2;

			if (n == n2)
				continue;

			i = mowgli_node_index(n, l);
			i2 = mowgli_node_index(n2, l);

			if ((result = comp(n, n2, opaque)) == 0)
				continue;
			else if (result < 0 && i > i2)
			{
				mowgli_node_delete(n, l);
				mowgli_node_add_before(n->data, n, l, n2);
			}
			else if (result > 0 && i < i2)
			{
				mowgli_node_delete(n, l);
				mowgli_node_add_after(n->data, n, l, n2);
			}
		}
	}
}
