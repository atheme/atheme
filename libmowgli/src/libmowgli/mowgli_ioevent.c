/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_ioevent.c: Portable I/O event layer.
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

#ifdef HAVE_EPOLL_CTL
# include <sys/epoll.h>
#endif

#ifdef HAVE_PORT_CREATE
# include <port.h>
#endif

mowgli_ioevent_handle_t *mowgli_ioevent_create(void)
{
	mowgli_ioevent_handle_t *self = mowgli_alloc(sizeof(mowgli_ioevent_handle_t));

#ifdef HAVE_EPOLL_CTL
	self->impldata = epoll_create(FD_SETSIZE);
#endif

#ifdef HAVE_PORT_CREATE
	self->impldata = port_create();
#endif

	return self;
}

void mowgli_ioevent_destroy(mowgli_ioevent_handle_t *self)
{
	return_if_fail(self != NULL);

#if defined(HAVE_EPOLL_CTL) || defined(HAVE_PORT_CREATE)
	close(self->impldata);
#endif

	mowgli_free(self);
}

int mowgli_ioevent_get(mowgli_ioevent_handle_t *self, mowgli_ioevent_t *buf, size_t bufsize, unsigned int delay)
{
#if defined HAVE_EPOLL_CTL || defined HAVE_PORT_CREATE
	int ret, iter;
#else
   int ret = -1;
#endif

#ifdef HAVE_EPOLL_CTL
	struct epoll_event events[bufsize];

	ret = epoll_wait(self->impldata, events, bufsize, delay);

	if (ret == -1)
		return ret;

	for (iter = 0; iter < ret; iter++)
	{
		buf[iter].ev_status = 0;
		buf[iter].ev_object = events[iter].data.fd;
		buf[iter].ev_opaque = events[iter].data.ptr;
		buf[iter].ev_source = MOWGLI_SOURCE_FD;

		if (events[iter].events & EPOLLIN)
			buf[iter].ev_status |= MOWGLI_POLLRDNORM;

		if (events[iter].events & EPOLLOUT)
			buf[iter].ev_status |= MOWGLI_POLLWRNORM;

		if (events[iter].events & EPOLLHUP)
			buf[iter].ev_status = MOWGLI_POLLHUP;

		if (events[iter].events & EPOLLERR)
			buf[iter].ev_status = MOWGLI_POLLERR;
	}
#endif

#ifdef HAVE_PORT_CREATE
	port_event_t events[bufsize];
	unsigned int nget = 1;
	struct timespec poll_time;

	poll_time.tv_sec = delay / 1000;
	poll_time.tv_nsec = (delay % 1000) * 1000000;

	ret = port_getn(self->impldata, events, bufsize, &nget, &poll_time);

	if (ret == -1)
		return ret;

	for (iter = 0; iter < nget; iter++)
	{
		buf[iter].ev_status = 0;
		buf[iter].ev_object = events[iter].portev_object;
		buf[iter].ev_opaque = events[iter].portev_user;
		buf[iter].ev_source = MOWGLI_SOURCE_FD;

		if (events[iter].portev_events & POLLRDNORM)
			buf[iter].ev_status |= MOWGLI_POLLRDNORM;

		if (events[iter].portev_events & POLLWRNORM)
			buf[iter].ev_status |= MOWGLI_POLLWRNORM;

		if (events[iter].portev_events & POLLHUP)
			buf[iter].ev_status = MOWGLI_POLLHUP;

		if (events[iter].portev_events & POLLERR)
			buf[iter].ev_status = MOWGLI_POLLERR;
	}

	ret = nget;
#endif

	return ret;
}

void mowgli_ioevent_associate(mowgli_ioevent_handle_t *self, mowgli_ioevent_source_t source, int object, unsigned int flags, void *opaque)
{
	int events = 0;

	if (source != MOWGLI_SOURCE_FD)
		return;

#ifdef HAVE_EPOLL_CTL
	{
		struct epoll_event ep_event = {};
		events = EPOLLONESHOT;

		if (flags & MOWGLI_POLLRDNORM)
			events |= EPOLLIN;

		if (flags & MOWGLI_POLLWRNORM)
			events |= EPOLLOUT;

		ep_event.events = events;
		ep_event.data.ptr = opaque;

		epoll_ctl(self->impldata, EPOLL_CTL_ADD, object, &ep_event);
	}
#endif

#ifdef HAVE_PORT_CREATE
#ifdef POLLRDNORM
	if (flags & MOWGLI_POLLRDNORM)
		events |= POLLRDNORM;
#endif

#ifdef EPOLLWRNORM
	if (flags & MOWGLI_POLLWRNORM)
		events |= EPOLLWRNORM;
#endif

	port_associate(self->impldata, PORT_SOURCE_FD, object, events, opaque);
#endif
}

void mowgli_ioevent_dissociate(mowgli_ioevent_handle_t *self, mowgli_ioevent_source_t source, int object)
{
	if (source != MOWGLI_SOURCE_FD)
		return;

#ifdef HAVE_EPOLL_CTL
	{
		struct epoll_event ep_event = {};

		epoll_ctl(self->impldata, EPOLL_CTL_DEL, object, &ep_event);
	}
#endif

#ifdef HAVE_PORT_CREATE
	port_dissociate(self->impldata, PORT_SOURCE_FD, object);
#endif
}

