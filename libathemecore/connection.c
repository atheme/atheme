/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * connection.c: Connection and I/O management
 */

#include <atheme.h>
#include "internal.h"

#ifdef MOWGLI_OS_WIN
# define EWOULDBLOCK WSAEWOULDBLOCK
# define EINPROGRESS WSAEINPROGRESS
# define SHUT_RDWR   SD_BOTH
#endif

mowgli_list_t connection_list;

static int
socket_setnonblocking(mowgli_descriptor_t sck)
{
#ifndef MOWGLI_OS_WIN
	int flags;

	slog(LG_DEBUG, "socket_setnonblocking(): setting file descriptor %d as non-blocking", sck);

	flags = fcntl(sck, F_GETFL, 0);
	flags |= O_NONBLOCK;

	if (fcntl(sck, F_SETFL, flags))
		return -1;

	return 0;
#else
	u_long yes = 1;

	ioctlsocket(sck, FIONBIO, &yes);
#endif
}

/*
 * connection_trampoline()
 *
 * inputs:
 *       mowgli.eventloop object
 *       mowgli.eventloop.pollable object
 *       poll direction
 *       user-specified data
 *
 * outputs:
 *       none
 *
 * side effects:
 *       whatever happens from the struct connection i/o handlers
 */
static void
connection_trampoline(mowgli_eventloop_t *eventloop, mowgli_eventloop_io_t *io,
	mowgli_eventloop_io_dir_t dir, void *userdata)
{
	struct connection *cptr = userdata;

	switch (dir) {
	case MOWGLI_EVENTLOOP_IO_READ:
		return cptr->read_handler(cptr);
	case MOWGLI_EVENTLOOP_IO_WRITE:
	case MOWGLI_EVENTLOOP_IO_ERROR:
		return cptr->write_handler(cptr);
	}
}

/*
 * connection_add()
 *
 * inputs:
 *       connection name, file descriptor, flag bitmask,
 *       read handler, write handler.
 *
 * outputs:
 *       a connection object for the connection information given.
 *
 * side effects:
 *       a connection is added to the socket queue.
 */
struct connection * ATHEME_FATTR_MALLOC
connection_add(const char *name, int fd, unsigned int flags,
	void (*read_handler)(struct connection *),
	void (*write_handler)(struct connection *))
{
	struct connection *cptr;

	if ((cptr = connection_find(fd)))
	{
		slog(LG_DEBUG, "connection_add(): connection %d is already registered as %s",
				fd, cptr->name);
		return NULL;
	}

	slog(LG_DEBUG, "connection_add(): adding connection '%s', fd=%d", name, fd);

	cptr = smalloc(sizeof *cptr);
	cptr->fd = fd;

	if (cptr->fd > -1)
	{
#ifndef MOWGLI_OS_WIN
		struct sockaddr_storage saddr;
		socklen_t saddrlen = sizeof saddr;
		memset(&saddr, 0x00, sizeof saddr);

		if (getpeername(cptr->fd, (struct sockaddr *) &saddr, &saddrlen) < 0)
		{
			slog(LG_ERROR, "connection_add(): %s: getpeername(2): %s", name, strerror(ioerrno()));
			sfree(cptr);
			return NULL;
		}

		if (saddrlen == sizeof(struct sockaddr_in) && saddr.ss_family == AF_INET)
		{
			struct sockaddr_in *const sa = (struct sockaddr_in *) &saddr;

			if (! inet_ntop(AF_INET, &sa->sin_addr, cptr->hbuf, sizeof cptr->hbuf))
			{
				slog(LG_ERROR, "connection_add(): %s: inet_ntop(3): %s", name, strerror(ioerrno()));
				sfree(cptr);
				return NULL;
			}
		}
		else if (saddrlen == sizeof(struct sockaddr_in6) && saddr.ss_family == AF_INET6)
		{
			struct sockaddr_in6 *const sa = (struct sockaddr_in6 *) &saddr;

			if (! inet_ntop(AF_INET6, &sa->sin6_addr, cptr->hbuf, sizeof cptr->hbuf))
			{
				slog(LG_ERROR, "connection_add(): %s: inet_ntop(3): %s", name, strerror(ioerrno()));
				sfree(cptr);
				return NULL;
			}
		}
		else
		{
			slog(LG_ERROR, "connection_add(): %s: unknown address family", name);
			sfree(cptr);
			return NULL;
		}
#endif
		socket_setnonblocking(cptr->fd);
	}

	cptr->pollslot = -1;
	cptr->flags = flags;
	cptr->first_recv = CURRTIME;
	cptr->last_recv = CURRTIME;
	cptr->pollable = mowgli_pollable_create(base_eventloop, fd, cptr);

	connection_setselect_read(cptr, read_handler);
	connection_setselect_write(cptr, write_handler);

	/* set connection name */
	mowgli_strlcpy(cptr->name, name, sizeof cptr->name);

	mowgli_node_add(cptr, mowgli_node_create(), &connection_list);

	return cptr;
}

/*
 * connection_find()
 *
 * inputs:
 *       the file descriptor to search by
 *
 * outputs:
 *       the struct connection object associated with that fd
 *
 * side effects:
 *       none
 */
struct connection *
connection_find(int fd)
{
	struct connection *cptr;
	mowgli_node_t *nptr;

	MOWGLI_ITER_FOREACH(nptr, connection_list.head)
	{
		cptr = nptr->data;

		if (cptr->fd == fd)
			return cptr;
	}

	return NULL;
}

/*
 * connection_close()
 *
 * inputs:
 *       the connection being closed.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the connection is closed.
 */
void
connection_close(struct connection *cptr)
{
	mowgli_node_t *nptr;
	int errno1, errno2;
#ifdef SO_ERROR
	socklen_t len = sizeof(errno2);
#endif

	if (!cptr)
	{
		slog(LG_DEBUG, "connection_close(): no connection to close!");
		return;
	}

	nptr = mowgli_node_find(cptr, &connection_list);
	if (!nptr)
	{
		slog(LG_ERROR, "connection_close(): connection %p is not registered!",
			cptr);
		return;
	}

	errno1 = ioerrno();
#ifdef SO_ERROR
	if (!getsockopt(cptr->fd, SOL_SOCKET, SO_ERROR, (char *) &errno2, (socklen_t *) &len))
	{
		if (errno2 != 0)
			errno1 = errno2;
	}
#endif

	if (errno1)
		slog(CF_IS_UPLINK(cptr) ? LG_ERROR : LG_DEBUG,
				"connection_close(): connection %s[%d] closed due to error %d (%s)",
				cptr->name, cptr->fd, errno1, strerror(errno1));

	if (cptr->close_handler)
		cptr->close_handler(cptr);

	/* close the fd */
	mowgli_pollable_destroy(base_eventloop, cptr->pollable);

	shutdown(cptr->fd, SHUT_RDWR);
	close(cptr->fd);

	mowgli_node_delete(nptr, &connection_list);
	mowgli_node_free(nptr);

	sendqrecvq_free(cptr);

	sfree(cptr);
}

/*
 * connection_close_children()
 *
 * inputs:
 *       a listener.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       connection_close() called on the connection itself and
 *       for all connections accepted on this listener
 */
void
connection_close_children(struct connection *cptr)
{
	mowgli_node_t *n;
	struct connection *cptr2;

	if (cptr == NULL)
		return;

	if (CF_IS_LISTENING(cptr))
	{
		MOWGLI_ITER_FOREACH(n, connection_list.head)
		{
			cptr2 = n->data;
			if (cptr2->listener == cptr)
				connection_close(cptr2);
		}
	}
	connection_close(cptr);
}

/* This one is only safe for use by connection_close_soon(),
 * it will cause infinite loops otherwise
 */
static void
empty_handler(struct connection *cptr)
{

}

/*
 * connection_close_soon()
 *
 * inputs:
 *       the connection being closed.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the connection is marked to be closed soon
 *       handlers reset
 *       close_handler called
 */
void
connection_close_soon(struct connection *cptr)
{
	if (cptr == NULL)
		return;
	cptr->flags |= CF_DEAD;
	/* these two cannot be NULL */
	cptr->read_handler = empty_handler;
	cptr->write_handler = empty_handler;
	cptr->recvq_handler = NULL;
	if (cptr->close_handler)
		cptr->close_handler(cptr);
	cptr->close_handler = NULL;
	cptr->listener = NULL;
}

/*
 * connection_close_soon_children()
 *
 * inputs:
 *       a listener.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       connection_close_soon() called on the connection itself and
 *       for all connections accepted on this listener
 */
void
connection_close_soon_children(struct connection *cptr)
{
	mowgli_node_t *n;
	struct connection *cptr2;

	if (cptr == NULL)
		return;

	if (CF_IS_LISTENING(cptr))
	{
		MOWGLI_ITER_FOREACH(n, connection_list.head)
		{
			cptr2 = n->data;
			if (cptr2->listener == cptr)
				connection_close_soon(cptr2);
		}
	}
	connection_close_soon(cptr);
}

/*
 * connection_close_all()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       none
 *
 * side effects:
 *       connection_close() called on all registered connections
 */
void
connection_close_all(void)
{
	mowgli_node_t *n, *tn;
	struct connection *cptr;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, connection_list.head)
	{
		cptr = n->data;
		connection_close(cptr);
	}
}

/*
 * connection_close_all_fds()
 *
 * Close all file descriptors of the connection subsystem in a child process.
 *
 * inputs:
 *       none
 *
 * outputs:
 *       none
 *
 * side effects:
 *       file descriptors belonging to all connections are closed
 *       no handlers called
 */
void
connection_close_all_fds(void)
{
	mowgli_node_t *n, *tn;
	struct connection *cptr;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, connection_list.head)
	{
		cptr = n->data;
		close(cptr->fd);
	}
}

/*
 * connection_open_tcp()
 *
 * inputs:
 *       hostname to connect to, vhost to use, port,
 *       read handler, write handler
 *
 * outputs:
 *       the struct connection on success, NULL on failure.
 *
 * side effects:
 *       a TCP/IP connection is opened to the host,
 *       and interest is registered in read/write events.
 */
struct connection *
connection_open_tcp(char *host, char *vhost, unsigned int port,
	void (*read_handler)(struct connection *),
	void (*write_handler)(struct connection *))
{
	struct connection *cptr;
	char buf[BUFSIZE];
	struct addrinfo *addr = NULL;
	int s, error;
	unsigned int optval;

	snprintf(buf, BUFSIZE, "tcp connection: %s", host);

	/* resolve it */
	if ((error = getaddrinfo(host, NULL, NULL, &addr)))
	{
		slog(LG_ERROR, "connection_open_tcp(): unable to resolve %s: %s", host, gai_strerror(error));
		return NULL;
	}

	/* some getaddrinfo() do this... */
	if (addr->ai_addr == NULL)
	{
		slog(LG_ERROR, "connection_open_tcp(): host %s is not resolveable.", vhost);
		freeaddrinfo(addr);
		return NULL;
	}

	if ((s = socket(addr->ai_family, SOCK_STREAM, 0)) < 0)
	{
		slog(LG_ERROR, "connection_open_tcp(): unable to create socket.");
		freeaddrinfo(addr);
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > claro_state.maxfd)
		claro_state.maxfd = s;

	if (vhost != NULL)
	{
		struct addrinfo hints, *bind_addr = NULL;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = addr->ai_family;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		if ((error = getaddrinfo(vhost, NULL, &hints, &bind_addr)))
		{
			slog(LG_ERROR, "connection_open_tcp(): cant resolve vhost %s: %s", vhost, gai_strerror(error));
			close(s);
			freeaddrinfo(addr);
			return NULL;
		}

		/* some getaddrinfo() do this... */
		if (bind_addr->ai_addr == NULL)
		{
			slog(LG_ERROR, "connection_open_tcp(): vhost %s is not resolveable.", vhost);
			close(s);
			freeaddrinfo(addr);
			freeaddrinfo(bind_addr);
			return NULL;
		}

		optval = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		{
			slog(LG_ERROR, "connection_open_tcp(): unable to bind to vhost %s: %s", vhost, strerror(ioerrno()));
			close(s);
			freeaddrinfo(addr);
			freeaddrinfo(bind_addr);
			return NULL;
		}

		if (bind(s, bind_addr->ai_addr, bind_addr->ai_addrlen) < 0)
		{
			slog(LG_ERROR, "connection_open_tcp(): unable to bind to vhost %s: %s", vhost, strerror(ioerrno()));
			close(s);
			freeaddrinfo(addr);
			freeaddrinfo(bind_addr);
			return NULL;
		}

		freeaddrinfo(bind_addr);
	}

	switch(addr->ai_family)
	{
	case AF_INET:
		((struct sockaddr_in *) addr->ai_addr)->sin_port = htons(port);
		break;
	case AF_INET6:
		((struct sockaddr_in6 *) addr->ai_addr)->sin6_port = htons(port);
		break;
	}

	if ((connect(s, addr->ai_addr, addr->ai_addrlen) == -1) && ioerrno() != EINPROGRESS && ioerrno() != EINTR && ioerrno() != EWOULDBLOCK)
	{
		if (vhost)
			slog(LG_ERROR, "connection_open_tcp(): failed to connect to %s using vhost %s: %d (%s)", host, vhost, ioerrno(), strerror(ioerrno()));
		else
			slog(LG_ERROR, "connection_open_tcp(): failed to connect to %s: %d (%s)", host, ioerrno(), strerror(ioerrno()));
		close(s);
		freeaddrinfo(addr);
		return NULL;
	}

	freeaddrinfo(addr);

	cptr = connection_add(buf, s, CF_CONNECTING, read_handler, write_handler);

	return cptr;
}

/*
 * connection_open_listener_tcp()
 *
 * inputs:
 *       host and port to listen on,
 *       accept handler
 *
 * outputs:
 *       the struct connection on success, NULL on failure.
 *
 * side effects:
 *       a TCP/IP connection is opened to the host,
 *       and interest is registered in read/write events.
 */
struct connection *
connection_open_listener_tcp(char *host, unsigned int port,
	void (*read_handler)(struct connection *))
{
	struct connection *cptr;
	char buf[BUFSIZE];
	struct addrinfo *addr;
	int s, error;
	unsigned int optval;

	/* resolve it */
	if ((error = getaddrinfo(host, NULL, NULL, &addr)))
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to resolve %s: %s", host, gai_strerror(error));
		return NULL;
	}

	/* some getaddrinfo() do this... */
	if (addr->ai_addr == NULL)
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): host %s is not resolveable.", host);
		freeaddrinfo(addr);
		return NULL;
	}

	if ((s = socket(addr->ai_family, SOCK_STREAM, 0)) < 0)
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to create socket.");
		freeaddrinfo(addr);
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > claro_state.maxfd)
		claro_state.maxfd = s;

	snprintf(buf, BUFSIZE, "listener: %s[%u]", host, port);

	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): error: %s", strerror(ioerrno()));
		freeaddrinfo(addr);
		close(s);
		return NULL;
	}

	switch(addr->ai_family)
	{
	case AF_INET:
		((struct sockaddr_in *) addr->ai_addr)->sin_port = htons(port);
		break;
	case AF_INET6:
		((struct sockaddr_in6 *) addr->ai_addr)->sin6_port = htons(port);
		break;
	}

	if (bind(s, addr->ai_addr, addr->ai_addrlen) < 0)
	{
		close(s);
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to bind listener %s[%u], errno [%d]", host, port,
			ioerrno());
		freeaddrinfo(addr);
		return NULL;
	}

	freeaddrinfo(addr);

	socket_setnonblocking(s);

	/* XXX we need to have some sort of handling for SOMAXCONN */
	if (listen(s, 5) < 0)
	{
		close(s);
		slog(LG_ERROR, "connection_open_listener_tcp(): error: %s", strerror(ioerrno()));
		return NULL;
	}

	cptr = connection_add(buf, s, CF_LISTENING, read_handler, NULL);

	return cptr;
}

/*
 * connection_accept_tcp()
 *
 * inputs:
 *       listener to accept from, read handler, write handler
 *
 * outputs:
 *       the struct connection on success, NULL on failure.
 *
 * side effects:
 *       a TCP/IP connection is accepted from the host,
 *       and interest is registered in read/write events.
 */
struct connection *
connection_accept_tcp(struct connection *cptr,
	void (*read_handler)(struct connection *),
	void (*write_handler)(struct connection *))
{
	char buf[BUFSIZE];
	struct connection *newptr;
	int s;

	if (!(s = accept(cptr->fd, NULL, NULL)))
	{
		slog(LG_INFO, "connection_accept_tcp(): accept failed");
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > claro_state.maxfd)
		claro_state.maxfd = s;

	socket_setnonblocking(s);

	mowgli_strlcpy(buf, "incoming connection", BUFSIZE);
	newptr = connection_add(buf, s, 0, read_handler, write_handler);
	newptr->listener = cptr;
	return newptr;
}

/*
 * connection_setselect_read()
 *
 * inputs:
 *       struct connection to register/deregister interest on,
 *       replacement handler (NULL = no interest)
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the read handler is changed for the struct connection.
 */
void
connection_setselect_read(struct connection *cptr,
	void (*read_handler)(struct connection *))
{
	cptr->read_handler = read_handler;
	mowgli_pollable_setselect(base_eventloop, cptr->pollable, MOWGLI_EVENTLOOP_IO_READ, cptr->read_handler != NULL ? connection_trampoline : NULL);
}

/*
 * connection_setselect_write()
 *
 * inputs:
 *       struct connection to register/deregister interest on,
 *       replacement handler (NULL = no interest)
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the write handler is changed for the struct connection.
 */
void
connection_setselect_write(struct connection *cptr,
	void (*write_handler)(struct connection *))
{
	cptr->write_handler = write_handler;
	mowgli_pollable_setselect(base_eventloop, cptr->pollable, MOWGLI_EVENTLOOP_IO_WRITE, cptr->write_handler != NULL ? connection_trampoline : NULL);
}

/*
 * connection_stats()
 *
 * inputs:
 *       callback function, data for callback function
 *
 * outputs:
 *       none
 *
 * side effects:
 *       callback function is called with a series of lines
 */
void
connection_stats(void (*stats_cb)(const char *, void *), void *privdata)
{
	mowgli_node_t *n;
	char buf[160];
	char buf2[20];

	MOWGLI_ITER_FOREACH(n, connection_list.head)
	{
		struct connection *c = (struct connection *) n->data;

		snprintf(buf, sizeof buf, "fd %-3d desc '%s'", c->fd, c->name);
		if (c->listener != NULL)
		{
			snprintf(buf2, sizeof buf2, " listener %d", c->listener->fd);
			mowgli_strlcat(buf, buf2, sizeof buf);
		}
		if (c->flags & (CF_CONNECTING | CF_DEAD | CF_NONEWLINE | CF_SEND_EOF | CF_SEND_DEAD))
		{
			mowgli_strlcat(buf, " status", sizeof buf);

			if (CF_IS_CONNECTING(c))
				mowgli_strlcat(buf, " connecting", sizeof buf);

			if (CF_IS_DEAD(c))
				mowgli_strlcat(buf, " dead", sizeof buf);

			if (CF_IS_NONEWLINE(c))
				mowgli_strlcat(buf, " nonewline", sizeof buf);

			if (CF_IS_SEND_DEAD(c))
				mowgli_strlcat(buf, " send_dead", sizeof buf);
			else if (CF_IS_SEND_EOF(c))
				mowgli_strlcat(buf, " send_eof", sizeof buf);
		}
		stats_cb(buf, privdata);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
