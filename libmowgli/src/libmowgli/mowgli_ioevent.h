/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_ioevent.h: Portable I/O event layer.
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

#ifndef __MOWGLI_IOEVENT_H__
#define __MOWGLI_IOEVENT_H__

typedef struct {
	int impldata;	/* implementation-specific data: this is almost always an FD */
} mowgli_ioevent_handle_t;

typedef enum {
	MOWGLI_SOURCE_FD = 1
} mowgli_ioevent_source_t;

typedef struct {
	mowgli_ioevent_source_t ev_source;
	unsigned int ev_status;
	int ev_object;
	void *ev_opaque;
} mowgli_ioevent_t;

#define MOWGLI_POLLRDNORM		0x01
#define MOWGLI_POLLWRNORM		0x02
#define MOWGLI_POLLHUP			0x04
#define MOWGLI_POLLERR			0x08

extern mowgli_ioevent_handle_t *mowgli_ioevent_create(void);
extern void mowgli_ioevent_destroy(mowgli_ioevent_handle_t *self);

extern int mowgli_ioevent_get(mowgli_ioevent_handle_t *self, mowgli_ioevent_t *buf, size_t bufsize, unsigned int delay);

extern void mowgli_ioevent_associate(mowgli_ioevent_handle_t *self, mowgli_ioevent_source_t source, int object, unsigned int flags, void *opaque);
extern void mowgli_ioevent_dissociate(mowgli_ioevent_handle_t *self, mowgli_ioevent_source_t source, int object);

#endif
