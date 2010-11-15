/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_spinlock.c: Spinlocks.
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

mowgli_spinlock_t *mowgli_spinlock_create(void)
{
	mowgli_spinlock_t *out = mowgli_alloc(sizeof(mowgli_spinlock_t));

	return out;
}

void mowgli_spinlock_lock(mowgli_spinlock_t *self, void *r, void *w)
{
	return_if_fail(self != NULL);

	if (r)
		mowgli_spinlock_wait(self, MOWGLI_SPINLOCK_READ);

	if (w)
		mowgli_spinlock_wait(self, MOWGLI_SPINLOCK_WRITE);

	if (r && (self->read_owner == NULL || self->read_owner == r))
		self->read_owner = r;

	if (w && (self->write_owner == NULL || self->write_owner == w))
		self->write_owner = w;
}

void mowgli_spinlock_unlock(mowgli_spinlock_t *self, void *r, void *w)
{
	return_if_fail(self != NULL);

	if (r && self->read_owner == r)
		self->read_owner = NULL;

	if (w && self->write_owner == w)
		self->write_owner = NULL;
}

void mowgli_spinlock_wait(mowgli_spinlock_t *self, mowgli_spinlock_lock_param_t param)
{
	return_if_fail(self != NULL)

	if (param == MOWGLI_SPINLOCK_READ)
		while (self->read_owner != NULL)
			usleep(1000);	/* XXX: we'll want a more threadsafe function eventually. */

	if (param == MOWGLI_SPINLOCK_WRITE)
		while (self->write_owner != NULL)
			usleep(1000);

	if (param == MOWGLI_SPINLOCK_READWRITE)
		while (self->write_owner != NULL || self->read_owner != NULL)
			usleep(1000);
}

void mowgli_spinlock_timed_wait(mowgli_spinlock_t *self, mowgli_spinlock_lock_param_t param, struct timeval *tv)
{
	struct timeval iter = {0};

	return_if_fail(self != NULL)
	return_if_fail(tv != NULL)

	if (param == MOWGLI_SPINLOCK_READ)
		while (self->read_owner != NULL && iter.tv_sec < tv->tv_sec && iter.tv_usec < tv->tv_usec)
		{
			gettimeofday(&iter, NULL);
			usleep(1000);	/* XXX: we'll want a more threadsafe function eventually. */
		}

	if (param == MOWGLI_SPINLOCK_WRITE)
		while (self->write_owner != NULL && iter.tv_sec < tv->tv_sec && iter.tv_usec < tv->tv_usec)
		{
			gettimeofday(&iter, NULL);
			usleep(1000);
		}

	if (param == MOWGLI_SPINLOCK_READWRITE)
		while ((self->write_owner != NULL || self->read_owner != NULL) && iter.tv_sec < tv->tv_sec && iter.tv_usec < tv->tv_usec)
		{
			gettimeofday(&iter, NULL);
			usleep(1000);	
		}
}
