/*
 * atheme-services: A collection of minimalist IRC services   
 * connection.c: Connection and I/O management
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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

#include "atheme.h"
#include "internal.h"
#include "datastream.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

mowgli_heap_t *connection_heap;

mowgli_list_t connection_list;

void init_netio(void)
{
	connection_heap = mowgli_heap_create(sizeof(connection_t), 16, BH_NOW);

	if (!connection_heap)
	{
		slog(LG_INFO, "init_netio(): blockheap failure");
		exit(EXIT_FAILURE);
	}
}

static int socket_setnonblocking(int sck)
{
	int flags;

	flags = fcntl(sck, F_GETFL, 0);
	flags |= O_NONBLOCK;

	if (fcntl(sck, F_SETFL, flags))
		return -1;

	return 0;
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
connection_t *connection_add(const char *name, int fd, unsigned int flags,
	void (*read_handler)(connection_t *),
	void (*write_handler)(connection_t *))
{
	connection_t *cptr;

	if ((cptr = connection_find(fd)))
	{
		slog(LG_DEBUG, "connection_add(): connection %d is already registered as %s",
				fd, cptr->name);
		return NULL;
	}

	slog(LG_DEBUG, "connection_add(): adding connection '%s', fd=%d", name, fd);

	cptr = mowgli_heap_alloc(connection_heap);

	cptr->fd = fd;
	cptr->pollslot = -1;
	cptr->flags = flags;
	cptr->first_recv = CURRTIME;
	cptr->last_recv = CURRTIME;

	cptr->read_handler = read_handler;
	cptr->write_handler = write_handler;

	/* set connection name */
	strlcpy(cptr->name, name, HOSTLEN);

	cptr->saddr_size = sizeof(cptr->saddr);
	getpeername(cptr->fd, &cptr->saddr.sa, &cptr->saddr_size);

	inet_ntop(cptr->saddr.sa.sa_family,
		  &cptr->saddr.sin6.sin6_addr,
		  cptr->hbuf, BUFSIZE);

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
 *       the connection_t object associated with that fd
 *
 * side effects:
 *       none
 */
connection_t *connection_find(int fd)
{
	connection_t *cptr;
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
void connection_close(connection_t *cptr)
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

	errno1 = errno;
#ifdef SO_ERROR
	if (!getsockopt(cptr->fd, SOL_SOCKET, SO_ERROR, (char *) &errno2, (socklen_t *) &len))
	{
		if (errno2 != 0)
			errno1 = errno2;
	}
#endif

	if (errno1)
		slog(cptr->flags & CF_UPLINK ? LG_ERROR : LG_DEBUG,
				"connection_close(): connection %s[%d] closed due to error %d (%s)",
				cptr->name, cptr->fd, errno1, strerror(errno1));

	if (cptr->close_handler)
		cptr->close_handler(cptr);

	/* close the fd */
	close(cptr->fd);

	mowgli_node_delete(nptr, &connection_list);
	mowgli_node_free(nptr);

	sendqrecvq_free(cptr);

	mowgli_heap_free(connection_heap, cptr);
}

/* This one is only safe for use by connection_close_soon(),
 * it will cause infinite loops otherwise
 */
static void empty_handler(connection_t *cptr)
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
void connection_close_soon(connection_t *cptr)
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
void connection_close_soon_children(connection_t *cptr)
{
	mowgli_node_t *n;
	connection_t *cptr2;

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
void connection_close_all(void)
{
	mowgli_node_t *n, *tn;
	connection_t *cptr;

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
void connection_close_all_fds(void)
{
	mowgli_node_t *n, *tn;
	connection_t *cptr;

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
 *       the connection_t on success, NULL on failure.
 *
 * side effects:
 *       a TCP/IP connection is opened to the host,
 *       and interest is registered in read/write events.
 */
connection_t *connection_open_tcp(char *host, char *vhost, unsigned int port,
	void (*read_handler)(connection_t *),
	void (*write_handler)(connection_t *))
{
	connection_t *cptr;
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
	
	if (!(s = socket(addr->ai_family, SOCK_STREAM, 0)))
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
		struct addrinfo *bind_addr = NULL;

		if ((error = getaddrinfo(vhost, NULL, NULL, &bind_addr)))
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
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

		if (bind(s, bind_addr->ai_addr, bind_addr->ai_addrlen) < 0)
		{
			slog(LG_ERROR, "connection_open_tcp(): unable to bind to vhost %s: %s", vhost, strerror(errno));
			close(s);
			freeaddrinfo(addr);
			freeaddrinfo(bind_addr);
			return NULL;
		}

		freeaddrinfo(bind_addr);
	}

	socket_setnonblocking(s);

	switch(addr->ai_family)
	{
	case AF_INET:
		((struct sockaddr_in *) addr->ai_addr)->sin_port = htons(port);
		break;
	case AF_INET6:
		((struct sockaddr_in6 *) addr->ai_addr)->sin6_port = htons(port);
		break;	
	}

	if ((connect(s, addr->ai_addr, addr->ai_addrlen) == -1) && errno != EINPROGRESS && errno != EINTR)
	{
		if (vhost)
			slog(LG_ERROR, "connection_open_tcp(): failed to connect to %s using vhost %s: %s", host, vhost, strerror(errno));
		else
			slog(LG_ERROR, "connection_open_tcp(): failed to connect to %s: %s", host, strerror(errno));
		close(s);
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
 *       the connection_t on success, NULL on failure.
 *
 * side effects:
 *       a TCP/IP connection is opened to the host,
 *       and interest is registered in read/write events.
 */
connection_t *connection_open_listener_tcp(char *host, unsigned int port,
	void (*read_handler)(connection_t *))
{
	connection_t *cptr;
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
	
	if (!(s = socket(addr->ai_family, SOCK_STREAM, 0)))
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to create socket.");
		freeaddrinfo(addr);
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > claro_state.maxfd)
		claro_state.maxfd = s;

	snprintf(buf, BUFSIZE, "listener: %s[%d]", host, port);

	optval = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

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
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to bind listener %s[%d], errno [%d]", host, port,
			errno);
		freeaddrinfo(addr);
		return NULL;
	}

	freeaddrinfo(addr);

	socket_setnonblocking(s);

	/* XXX we need to have some sort of handling for SOMAXCONN */
	if (listen(s, 5) < 0)
	{
		close(s);
		slog(LG_ERROR, "connection_open_listener_tcp(): error: %s", strerror(errno));
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
 *       the connection_t on success, NULL on failure.
 *
 * side effects:
 *       a TCP/IP connection is accepted from the host,
 *       and interest is registered in read/write events.
 */
connection_t *connection_accept_tcp(connection_t *cptr,
	void (*read_handler)(connection_t *),
	void (*write_handler)(connection_t *))
{
	char buf[BUFSIZE];
	connection_t *newptr;
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

	strlcpy(buf, "incoming connection", BUFSIZE);
	newptr = connection_add(buf, s, 0, read_handler, write_handler);
	newptr->listener = cptr;
	return newptr;
}

/*
 * connection_setselect_read()
 *
 * inputs:
 *       connection_t to register/deregister interest on,
 *       replacement handler (NULL = no interest)
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the read handler is changed for the connection_t.
 */
void connection_setselect_read(connection_t *cptr,
	void (*read_handler)(connection_t *))
{
	cptr->read_handler = read_handler;
}

/*
 * connection_setselect_write()
 *
 * inputs:
 *       connection_t to register/deregister interest on,
 *       replacement handler (NULL = no interest)
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the write handler is changed for the connection_t.
 */
void connection_setselect_write(connection_t *cptr,
	void (*write_handler)(connection_t *))
{
	cptr->write_handler = write_handler;
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
void connection_stats(void (*stats_cb)(const char *, void *), void *privdata)
{
	mowgli_node_t *n;
	char buf[160];
	char buf2[20];

	MOWGLI_ITER_FOREACH(n, connection_list.head)
	{
		connection_t *c = (connection_t *) n->data;

		snprintf(buf, sizeof buf, "fd %-3d desc '%s'", c->fd, c->flags & CF_UPLINK ? "uplink" : c->flags & CF_LISTENING ? "listener" : "misc");
		if (c->listener != NULL)
		{
			snprintf(buf2, sizeof buf2, " listener %d", c->listener->fd);
			strlcat(buf, buf2, sizeof buf);
		}
		if (c->flags & (CF_CONNECTING | CF_DEAD | CF_NONEWLINE | CF_SEND_EOF | CF_SEND_DEAD))
		{
			strlcat(buf, " status", sizeof buf);
			if (c->flags & CF_CONNECTING)
				strlcat(buf, " connecting", sizeof buf);
			if (c->flags & CF_DEAD)
				strlcat(buf, " dead", sizeof buf);
			if (c->flags & CF_NONEWLINE)
				strlcat(buf, " nonewline", sizeof buf);
			if (c->flags & CF_SEND_DEAD)
				strlcat(buf, " send_dead", sizeof buf);
			else if (c->flags & CF_SEND_EOF)
				strlcat(buf, " send_eof", sizeof buf);
		}
		stats_cb(buf, privdata);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
