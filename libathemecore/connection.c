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
#  define EINPROGRESS   WSAEINPROGRESS
#  define EWOULDBLOCK   WSAEWOULDBLOCK
#  define SHUT_RDWR     SD_BOTH
#endif

#define CONNECTION_STATUS_FLAGS (CF_CONNECTING | CF_LISTENING | CF_DEAD | CF_NONEWLINE | CF_SEND_EOF | CF_SEND_DEAD)

mowgli_list_t connection_list;

static bool
connection_addr_satostr(const char *const restrict func, const struct sockaddr *const restrict sa,
                        char dst[const restrict static CONNECTION_ADDRSTRLEN])
{
	char addr[INET6_ADDRSTRLEN];
	unsigned int port;
	const void *src;

	switch (sa->sa_family)
	{
		case AF_INET:
			port = (unsigned int) ntohs(((struct sockaddr_in *) sa)->sin_port);
			src = &((struct sockaddr_in *) sa)->sin_addr;
			break;

		case AF_INET6:
			port = (unsigned int) ntohs(((struct sockaddr_in6 *) sa)->sin6_port);
			src = &((struct sockaddr_in6 *) sa)->sin6_addr;
			break;

		default:
			(void) slog(LG_ERROR, "%s: unknown address family", func);
			return false;
	}
	if (! inet_ntop(sa->sa_family, src, addr, sizeof addr))
	{
		(void) slog(LG_ERROR, "%s: inet_ntop(3): %s", func, strerror(ioerrno()));
		return false;
	}

	(void) memset(dst, 0x00, CONNECTION_ADDRSTRLEN);
	(void) snprintf(dst, CONNECTION_ADDRSTRLEN, "[%s]:%u", addr, port);
	return true;
}

static void
connection_empty_handler(struct connection *ATHEME_VATTR_UNUSED cptr)
{
	/* This handler is only safe for use by connection_close_soon();
	 * it will cause infinite loops otherwise.
	 */
}

static inline bool
connection_ignore_errno(const int error)
{
	if (error == EINTR)
		return true;

	if (error == EINPROGRESS)
		return true;

	if (error == EWOULDBLOCK)
		return true;

	return false;
}

static bool
socket_setcloexec(mowgli_descriptor_t sck)
{
	(void) slog(LG_DEBUG, "%s: setting file descriptor %d as close-on-exec", MOWGLI_FUNC_NAME, sck);

#ifdef MOWGLI_OS_WIN
	(void) SetHandleInformation((HANDLE) sck, HANDLE_FLAG_INHERIT, 0);
#else
	const int flags = fcntl(sck, F_GETFD, NULL);

	if (flags == -1 || fcntl(sck, F_SETFD, (flags | FD_CLOEXEC)) == -1)
		return false;
#endif

	return true;
}

static bool
socket_setnonblocking(mowgli_descriptor_t sck)
{
	(void) slog(LG_DEBUG, "%s: setting file descriptor %d as non-blocking", MOWGLI_FUNC_NAME, sck);

#ifdef MOWGLI_OS_WIN
	const unsigned long yes = 1;

	(void) ioctlsocket(sck, FIONBIO, &yes);
#else
	const int flags = fcntl(sck, F_GETFL, 0);

	if (flags == -1 || fcntl(sck, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		(void) slog(LG_ERROR, "%s: fcntl(2): %s", MOWGLI_FUNC_NAME, strerror(errno));
		return false;
	}
#endif

	return true;
}

static void
connection_trampoline(mowgli_eventloop_t *const restrict eventloop, mowgli_eventloop_io_t *const restrict io,
                      const mowgli_eventloop_io_dir_t dir, void *const restrict userdata)
{
	struct connection *const cptr = userdata;

	switch (dir)
	{
		case MOWGLI_EVENTLOOP_IO_READ:
			(void) cptr->read_handler(cptr);
			return;

		case MOWGLI_EVENTLOOP_IO_WRITE:
		case MOWGLI_EVENTLOOP_IO_ERROR:
			(void) cptr->write_handler(cptr);
			return;
	}
}

struct connection * ATHEME_FATTR_MALLOC
connection_add(const char *const restrict name, const int fd, const unsigned int flags,
               const connection_evhandler read_handler, const connection_evhandler write_handler)
{
	return_null_if_fail(fd >= 0);
	return_null_if_fail(name != NULL);
	return_null_if_fail(read_handler != NULL || write_handler != NULL);

	struct connection *cptr;

	if ((cptr = connection_find(fd)))
	{
		(void) slog(LG_DEBUG, "%s: connection %d is already registered (as '%s')",
		                      MOWGLI_FUNC_NAME, fd, cptr->name);
		return NULL;
	}

	(void) slog(LG_DEBUG, "%s: adding connection %d ('%s')", MOWGLI_FUNC_NAME, fd, name);

	if (! socket_setcloexec(fd))
		return NULL;

	if (! socket_setnonblocking(fd))
		return NULL;

	cptr                = smalloc(sizeof *cptr);
	cptr->fd            = fd;
	cptr->flags         = flags;
	cptr->first_recv    = CURRTIME;
	cptr->last_recv     = CURRTIME;
	cptr->pollable      = mowgli_pollable_create(base_eventloop, fd, cptr);

	(void) connection_setselect_read(cptr, read_handler);
	(void) connection_setselect_write(cptr, write_handler);
	(void) mowgli_strlcpy(cptr->name, name, sizeof cptr->name);
	(void) mowgli_node_add(cptr, &cptr->node, &connection_list);

	return cptr;
}

struct connection *
connection_find(const int fd)
{
	mowgli_node_t *n;
	MOWGLI_ITER_FOREACH(n, connection_list.head)
	{
		struct connection *const cptr = n->data;

		if (cptr->fd == fd)
			return cptr;
	}

	return NULL;
}

void
connection_close(struct connection *const restrict cptr)
{
	if (! cptr)
	{
		(void) slog(LG_DEBUG, "%s: no connection to close!", MOWGLI_FUNC_NAME);
		return;
	}

	mowgli_node_t *const n = mowgli_node_find(cptr, &connection_list);
	if (! n)
	{
		(void) slog(LG_ERROR, "%s: fd %d is not registered!", MOWGLI_FUNC_NAME, cptr->fd);
		return;
	}

	int errsv = ioerrno();

#if defined(SOL_SOCKET) && defined(SO_ERROR)
	int errsck = 0;
	socklen_t errlen = sizeof errsck;
	if (getsockopt(cptr->fd, SOL_SOCKET, SO_ERROR, &errsck, &errlen) == 0 && errsck != 0)
		errsv = errsck;
#endif

	if (errsv)
		(void) slog(CF_IS_UPLINK(cptr) ? LG_ERROR : LG_DEBUG, "%s: fd %d ('%s') closed due to error %d (%s)",
		            MOWGLI_FUNC_NAME, cptr->fd, cptr->name, errsv, strerror(errsv));

	if (cptr->close_handler)
		(void) cptr->close_handler(cptr);

	(void) mowgli_pollable_destroy(base_eventloop, cptr->pollable);
	(void) mowgli_node_delete(&cptr->node, &connection_list);
	(void) sendqrecvq_free(cptr);
	(void) shutdown(cptr->fd, SHUT_RDWR);
	(void) close(cptr->fd);
	(void) sfree(cptr);
}

void
connection_close_children(struct connection *const restrict cptr)
{
	if (! cptr)
		return;

	if (CF_IS_LISTENING(cptr))
	{
		mowgli_node_t *n, *tn;
		MOWGLI_ITER_FOREACH_SAFE(n, tn, connection_list.head)
		{
			struct connection *const child = n->data;

			if (child->listener == cptr)
				(void) connection_close(child);
		}
	}

	(void) connection_close(cptr);
}

void
connection_close_soon(struct connection *const restrict cptr)
{
	if (! cptr)
		return;

	if (cptr->close_handler)
		(void) cptr->close_handler(cptr);

	cptr->read_handler      = &connection_empty_handler;
	cptr->write_handler     = &connection_empty_handler;
	cptr->recvq_handler     = NULL;
	cptr->close_handler     = NULL;
	cptr->listener          = NULL;
	cptr->flags            |= CF_DEAD;
}

void
connection_close_soon_children(struct connection *const restrict cptr)
{
	if (! cptr)
		return;

	if (CF_IS_LISTENING(cptr))
	{
		mowgli_node_t *n, *tn;
		MOWGLI_ITER_FOREACH_SAFE(n, tn, connection_list.head)
		{
			struct connection *const child = n->data;

			if (child->listener == cptr)
				(void) connection_close_soon(child);
		}
	}

	(void) connection_close_soon(cptr);
}

void
connection_close_all(void)
{
	mowgli_node_t *n, *tn;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, connection_list.head)
		(void) connection_close(n->data);
}

struct connection *
connection_open_tcp(const char *const restrict host, const char *const restrict vhost, const unsigned int port,
                    const connection_evhandler read_handler, const connection_evhandler write_handler)
{
	return_null_if_fail(host != NULL);
	return_null_if_fail(port > 0 && port < 65536);
	return_null_if_fail(read_handler != NULL || write_handler != NULL);

	char sport[16];
	(void) snprintf(sport, sizeof sport, "%u", port);

	struct addrinfo *host_addr = NULL;
	const struct addrinfo host_hints = {
		.ai_flags       = AI_NUMERICSERV,
		.ai_family      = AF_UNSPEC,
		.ai_socktype    = SOCK_STREAM,
		.ai_protocol    = IPPROTO_TCP,
	};

	const int gai_host_errno = getaddrinfo(host, sport, &host_hints, &host_addr);

	if (gai_host_errno)
	{
		(void) slog(LG_ERROR, "%s: unable to resolve '%s': getaddrinfo(3): %s",
		                      MOWGLI_FUNC_NAME, host, gai_strerror(gai_host_errno));
		return NULL;
	}
	if (! (host_addr && host_addr->ai_addr))
	{
		(void) slog(LG_ERROR, "%s: unable to resolve '%s' (unknown error)", MOWGLI_FUNC_NAME, host);

		if (host_addr)
			(void) freeaddrinfo(host_addr);

		return NULL;
	}

	const int fd = socket(host_addr->ai_family, host_addr->ai_socktype, host_addr->ai_protocol);

	if (fd == -1)
	{
		(void) slog(LG_ERROR, "%s: unable to connect to '%s': socket(2): %s",
		                      MOWGLI_FUNC_NAME, host, strerror(ioerrno()));

		(void) freeaddrinfo(host_addr);
		return NULL;
	}

	if (! socket_setnonblocking(fd))
	{
		(void) freeaddrinfo(host_addr);
		(void) close(fd);
		return NULL;
	}

	char vhost_hbuf[CONNECTION_ADDRSTRLEN];
	char host_hbuf[CONNECTION_ADDRSTRLEN];
	char name[BUFSIZE];

	if (! connection_addr_satostr(MOWGLI_FUNC_NAME, host_addr->ai_addr, host_hbuf))
	{
		(void) freeaddrinfo(host_addr);
		(void) close(fd);
		return NULL;
	}

	if (vhost)
	{
		struct addrinfo *vhost_addr = NULL;
		const struct addrinfo vhost_hints = {
			.ai_flags       = AI_PASSIVE,
			.ai_family      = host_addr->ai_family,
			.ai_socktype    = host_addr->ai_socktype,
			.ai_protocol    = host_addr->ai_protocol,
		};

		const int gai_vhost_errno = getaddrinfo(vhost, NULL, &vhost_hints, &vhost_addr);

		if (gai_vhost_errno)
		{
			(void) slog(LG_ERROR, "%s: unable to connect to '%s': unable to resolve '%s': "
			                      "getaddrinfo(3): %s", MOWGLI_FUNC_NAME, host, vhost,
			                      gai_strerror(gai_vhost_errno));

			(void) freeaddrinfo(host_addr);
			(void) close(fd);
			return NULL;
		}
		if (! (vhost_addr && vhost_addr->ai_addr))
		{
			(void) slog(LG_ERROR, "%s: unable to connect to '%s': unable to resolve '%s' "
			                      "(unknown error)", MOWGLI_FUNC_NAME, host, vhost);

			if (vhost_addr)
				(void) freeaddrinfo(vhost_addr);

			(void) freeaddrinfo(host_addr);
			(void) close(fd);
			return NULL;
		}

#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
		const unsigned int optval = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1)
		{
			(void) slog(LG_ERROR, "%s: unable to connect to '%s': unable to bind to '%s': "
			                      "setsockopt(2): %s", MOWGLI_FUNC_NAME, host, vhost,
			                      strerror(ioerrno()));

			(void) freeaddrinfo(vhost_addr);
			(void) freeaddrinfo(host_addr);
			(void) close(fd);
			return NULL;
		}
#endif /* SOL_SOCKET && SO_REUSEADDR */

		if (bind(fd, vhost_addr->ai_addr, vhost_addr->ai_addrlen) == -1)
		{
			(void) slog(LG_ERROR, "%s: unable to connect to '%s': unable to bind to '%s': "
			                      "bind(2): %s", MOWGLI_FUNC_NAME, host, vhost,
			                      strerror(ioerrno()));

			(void) freeaddrinfo(vhost_addr);
			(void) freeaddrinfo(host_addr);
			(void) close(fd);
			return NULL;
		}

		if (! connection_addr_satostr(MOWGLI_FUNC_NAME, vhost_addr->ai_addr, vhost_hbuf))
		{
			(void) freeaddrinfo(vhost_addr);
			(void) freeaddrinfo(host_addr);
			(void) close(fd);
			return NULL;
		}

		(void) freeaddrinfo(vhost_addr);
		(void) snprintf(name, sizeof name, "%s -> %s", vhost_hbuf, host_hbuf);
	}
	else
		(void) snprintf(name, sizeof name, "[::]:0 -> %s", host_hbuf);

	if (connect(fd, host_addr->ai_addr, host_addr->ai_addrlen) == -1 && ! connection_ignore_errno(ioerrno()))
	{
		if (vhost)
			(void) slog(LG_ERROR, "%s: unable to connect from '%s' (%s) to '%s' (%s): connect(2): %s",
			                      MOWGLI_FUNC_NAME, vhost, vhost_hbuf, host, host_hbuf,
			                      strerror(ioerrno()));
		else
			(void) slog(LG_ERROR, "%s: unable to connect to '%s' (%s): connect(2): %s",
			                      MOWGLI_FUNC_NAME, host, host_hbuf, strerror(ioerrno()));

		(void) freeaddrinfo(host_addr);
		(void) close(fd);
		return NULL;
	}

	(void) freeaddrinfo(host_addr);

	return connection_add(name, fd, CF_CONNECTING, read_handler, write_handler);
}

struct connection *
connection_open_listener_tcp(const char *const restrict host, const unsigned int port,
                             const connection_evhandler read_handler)
{
	return_null_if_fail(host != NULL);
	return_null_if_fail(port > 0 && port < 65536);
	return_null_if_fail(read_handler != NULL);

	char sport[16];
	(void) snprintf(sport, sizeof sport, "%u", port);

	struct addrinfo *addr = NULL;
	const struct addrinfo hints = {
		.ai_flags       = AI_PASSIVE | AI_NUMERICSERV,
		.ai_family      = AF_UNSPEC,
		.ai_socktype    = SOCK_STREAM,
		.ai_protocol    = IPPROTO_TCP,
	};

	const int gai_errno = getaddrinfo(host, sport, &hints, &addr);

	if (gai_errno)
	{
		(void) slog(LG_ERROR, "%s: unable to resolve '%s': getaddrinfo(3): %s",
		                      MOWGLI_FUNC_NAME, host, gai_strerror(gai_errno));
		return NULL;
	}
	if (! (addr && addr->ai_addr))
	{
		(void) slog(LG_ERROR, "%s: unable to resolve '%s' (unknown error)", MOWGLI_FUNC_NAME, host);

		if (addr)
			(void) freeaddrinfo(addr);

		return NULL;
	}

	const int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

	if (fd == -1)
	{
		(void) slog(LG_ERROR, "%s: unable to listen on '%s': socket(2): %s",
		                      MOWGLI_FUNC_NAME, host, strerror(ioerrno()));

		(void) freeaddrinfo(addr);
		return NULL;
	}

	char hbuf[CONNECTION_ADDRSTRLEN];

	if (! connection_addr_satostr(MOWGLI_FUNC_NAME, addr->ai_addr, hbuf))
	{
		(void) freeaddrinfo(addr);
		(void) close(fd);
		return NULL;
	}

#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
	const unsigned int optval = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1)
	{
		(void) slog(LG_ERROR, "%s: unable to listen on '%s': setsockopt(2): %s",
		                      MOWGLI_FUNC_NAME, host, strerror(ioerrno()));

		(void) freeaddrinfo(addr);
		(void) close(fd);
		return NULL;
	}
#endif /* SOL_SOCKET && SO_REUSEADDR */

	if (bind(fd, addr->ai_addr, addr->ai_addrlen) == -1)
	{
		(void) slog(LG_ERROR, "%s: unable to listen on '%s': bind(2): %s",
		                      MOWGLI_FUNC_NAME, host, strerror(ioerrno()));

		(void) freeaddrinfo(addr);
		(void) close(fd);
		return NULL;
	}

	(void) freeaddrinfo(addr);

	if (listen(fd, 5) == -1)
	{
		(void) slog(LG_ERROR, "%s: unable to listen on '%s': listen(2): %s",
		                      MOWGLI_FUNC_NAME, host, strerror(ioerrno()));

		(void) close(fd);
		return NULL;
	}

	char name[BUFSIZE];
	(void) snprintf(name, sizeof name, "%s", hbuf);

	return connection_add(name, fd, CF_LISTENING, read_handler, NULL);
}

struct connection *
connection_accept_tcp(struct connection *const restrict cptr, const connection_evhandler read_handler,
                      const connection_evhandler write_handler)
{
	struct sockaddr_storage peer_ss;
	socklen_t peer_ss_len = sizeof peer_ss;
	const int fd = accept(cptr->fd, (struct sockaddr *) &peer_ss, &peer_ss_len);
	if (fd == -1)
	{
		(void) slog(LG_ERROR, "%s: accept(2): %s", MOWGLI_FUNC_NAME, strerror(ioerrno()));
		return NULL;
	}

	struct sockaddr_storage sock_ss;
	socklen_t sock_ss_len = sizeof sock_ss;
	if (getsockname(fd, (struct sockaddr *) &sock_ss, &sock_ss_len) == -1)
	{
		(void) slog(LG_ERROR, "%s: getsockname(2): %s", MOWGLI_FUNC_NAME, strerror(ioerrno()));
		(void) close(fd);
		return NULL;
	}

	char peer_hbuf[CONNECTION_ADDRSTRLEN];
	char sock_hbuf[CONNECTION_ADDRSTRLEN];
	if (! connection_addr_satostr(MOWGLI_FUNC_NAME, (struct sockaddr *) &peer_ss, peer_hbuf))
	{
		(void) close(fd);
		return NULL;
	}
	if (! connection_addr_satostr(MOWGLI_FUNC_NAME, (struct sockaddr *) &sock_ss, sock_hbuf))
	{
		(void) close(fd);
		return NULL;
	}

	char name[BUFSIZE];
	(void) snprintf(name, sizeof name, "%s <- %s", sock_hbuf, peer_hbuf);

	struct connection *const newptr = connection_add(name, fd, CF_CONNECTED, read_handler, write_handler);

	if (newptr)
		newptr->listener = cptr;

	return newptr;
}

void
connection_setselect_read(struct connection *const restrict cptr, const connection_evhandler read_handler)
{
	cptr->read_handler = read_handler;

	(void) mowgli_pollable_setselect(base_eventloop, cptr->pollable, MOWGLI_EVENTLOOP_IO_READ,
	                                 read_handler ? &connection_trampoline : NULL);
}

void
connection_setselect_write(struct connection *const restrict cptr, const connection_evhandler write_handler)
{
	cptr->write_handler = write_handler;

	(void) mowgli_pollable_setselect(base_eventloop, cptr->pollable, MOWGLI_EVENTLOOP_IO_WRITE,
	                                 write_handler ? &connection_trampoline : NULL);
}

void
connection_stats(void (*const stats_cb)(const char *, void *), void *const restrict privdata)
{
	mowgli_node_t *n;
	MOWGLI_ITER_FOREACH(n, connection_list.head)
	{
		const struct connection *const cptr = n->data;

		char buf[BUFSIZE];

		(void) snprintf(buf, sizeof buf, "fd %d desc '%s'", cptr->fd, cptr->name);

		if (CF_IS_UPLINK(cptr))
			(void) mowgli_strlcat(buf, " (uplink)", sizeof buf);

		if (cptr->listener)
		{
			char listenbuf[BUFSIZE];

			(void) snprintf(listenbuf, sizeof listenbuf, " listener %d", cptr->listener->fd);
			(void) mowgli_strlcat(buf, listenbuf, sizeof buf);
		}
		if (cptr->flags & CONNECTION_STATUS_FLAGS)
		{
			(void) mowgli_strlcat(buf, " status", sizeof buf);

			if (CF_IS_CONNECTING(cptr))
				(void) mowgli_strlcat(buf, " connecting", sizeof buf);

			if (CF_IS_LISTENING(cptr))
				(void) mowgli_strlcat(buf, " listening", sizeof buf);

			if (CF_IS_DEAD(cptr))
				(void) mowgli_strlcat(buf, " dead", sizeof buf);

			if (CF_IS_NONEWLINE(cptr))
				(void) mowgli_strlcat(buf, " nonewline", sizeof buf);

			if (CF_IS_SEND_DEAD(cptr))
				(void) mowgli_strlcat(buf, " send_dead", sizeof buf);
			else if (CF_IS_SEND_EOF(cptr))
				(void) mowgli_strlcat(buf, " send_eof", sizeof buf);
		}

		(void) stats_cb(buf, privdata);
	}
}
