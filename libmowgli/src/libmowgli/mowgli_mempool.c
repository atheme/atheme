/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_mempool.c: Memory pooling.
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

/* visibility of this object is not available to the outside */
struct mowgli_mempool_t_ {
	mowgli_list_t stack;
	mowgli_destructor_t destructor;
#ifdef NOTYET
	mowgli_mutex_t *mutex;
#endif
};

typedef struct {
	void *addr;
	int refcount;
	mowgli_node_t node;
} mowgli_mempool_elem_t;

mowgli_mempool_t *mowgli_mempool_with_custom_destructor(mowgli_destructor_t destructor)
{
	mowgli_mempool_t *pool;

	pool = mowgli_alloc(sizeof(mowgli_mempool_t));
	pool->destructor = destructor;
#ifdef NOTYET
	pool->mutex = mowgli_mutex_create();
#endif
	return pool;
}

mowgli_mempool_t *mowgli_mempool_create(void)
{
	return mowgli_mempool_with_custom_destructor(mowgli_free);
}

void *mowgli_mempool_add(mowgli_mempool_t * pool, void * ptr)
{
	mowgli_mempool_elem_t *e = mowgli_alloc(sizeof(mowgli_mempool_elem_t));

	e->addr = ptr;
	e->refcount = 1;

#ifdef NOTYET
	mowgli_mutex_lock(pool->mutex);
#endif
	mowgli_node_add(e, &e->node, &pool->stack);
#ifdef NOTYET
	mowgli_mutex_unlock(pool->mutex);
#endif
	return ptr;
}

void *
mowgli_mempool_allocate(mowgli_mempool_t * pool, size_t sz)
{
	void * addr;

#ifdef NOTYET
	mowgli_mutex_lock(pool->mutex);
#endif
	addr = mowgli_alloc(sz);
	mowgli_node_add(addr, mowgli_node_create(), &pool->stack);
#ifdef NOTYET
	mowgli_mutex_unlock(pool->mutex);
#endif
	return addr;
}

void
mowgli_mempool_sustain(mowgli_mempool_t * pool, void * addr)
{
	mowgli_node_t *n, *tn;
	mowgli_mempool_elem_t *e;

#ifdef NOTYET
	mowgli_mutex_lock(pool->mutex);
#endif

	MOWGLI_LIST_FOREACH_SAFE(n, tn, pool->stack.head)
	{
		e = (mowgli_mempool_elem_t *) n->data;

		if (e->addr == addr)
			++e->refcount;
	}

#ifdef NOTYET
	mowgli_mutex_unlock(pool->mutex);
#endif
}

void
mowgli_mempool_release(mowgli_mempool_t * pool, void * addr)
{
	mowgli_node_t *n, *tn;
	mowgli_mempool_elem_t *e;

#ifdef NOTYET
	mowgli_mutex_lock(pool->mutex);
#endif

	MOWGLI_LIST_FOREACH_SAFE(n, tn, pool->stack.head)
	{
		e = (mowgli_mempool_elem_t *) n->data;

		if (e->addr == addr && --e->refcount == 0)
		{
			mowgli_node_delete(n, &pool->stack);
			pool->destructor(addr);
			mowgli_free(e);
		}
	}

#ifdef NOTYET
	mowgli_mutex_unlock(pool->mutex);
#endif
}

static void
mowgli_mempool_cleanup_nolock(mowgli_mempool_t * pool)
{
	mowgli_node_t *n, *tn;

	MOWGLI_LIST_FOREACH_SAFE(n, tn, pool->stack.head)
	{
		mowgli_mempool_elem_t *e = (mowgli_mempool_elem_t *) n->data;

		/* don't care about refcounting here. we're killing the entire pool. */
		mowgli_log("mowgli_mempool_t<%p> element at %p was not released until cleanup (refcount: %d)", pool, e->addr, e->refcount);
		pool->destructor(e->addr);
		mowgli_free(e);

		mowgli_node_delete(n, &pool->stack);
	}
}

void
mowgli_mempool_cleanup(mowgli_mempool_t * pool)
{
#ifdef NOTYET
	mowgli_mutex_lock(pool->mutex);
#endif
	mowgli_mempool_cleanup_nolock(pool);
#ifdef NOTYET
	mowgli_mutex_unlock(pool->mutex);
#endif
}

void
mowgli_mempool_destroy(mowgli_mempool_t * pool)
{
#ifdef NOTYET
	mowgli_mutex_lock(pool->mutex);
#endif

	mowgli_mempool_cleanup_nolock(pool);

#ifdef NOTYET
	mowgli_mutex_unlock(pool->mutex);

	mowgli_mutex_free(pool->mutex);
#endif

	mowgli_free(pool);
}

char *
mowgli_mempool_strdup(mowgli_mempool_t * pool, char * src)
{
	char *out;
	size_t sz = strlen(src) + 1;

	out = mowgli_mempool_allocate(pool, sz);
	strncpy(out, src, sz);

	return out;
}
