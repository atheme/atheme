/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_spinlock.h: Spinlocks.
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

#ifndef __MOWGLI_SPINLOCK_H__
#define __MOWGLI_SPINLOCK_H__

typedef struct {
	void *read_owner;	/* opaque data representing a spinlock's owner */
	void *write_owner;	/* opaque data representing a spinlock's owner */
} mowgli_spinlock_t;

typedef enum {
	MOWGLI_SPINLOCK_READ,
	MOWGLI_SPINLOCK_WRITE,
	MOWGLI_SPINLOCK_READWRITE
} mowgli_spinlock_lock_param_t;

extern mowgli_spinlock_t *mowgli_spinlock_create(void);
extern void mowgli_spinlock_lock(mowgli_spinlock_t *self, void *r, void *w);
extern void mowgli_spinlock_unlock(mowgli_spinlock_t *self, void *r, void *w);
extern void mowgli_spinlock_wait(mowgli_spinlock_t *self, mowgli_spinlock_lock_param_t param);
extern void mowgli_spinlock_timed_wait(mowgli_spinlock_t *self, mowgli_spinlock_lock_param_t param, struct timeval *tv);

#endif
