/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Connection and I/O management.
 *
 * $Id: connection.c 8079 2007-04-02 17:37:39Z nenolod $
 */

#include "atheme.h"
#include "internal.h"

static BlockHeap *sa_heap;
static BlockHeap *connection_heap;

list_t connection_list;

void init_netio(void)
{
	connection_heap = BlockHeapCreate(sizeof(connection_t), 16);
	sa_heap = BlockHeapCreate(sizeof(struct sockaddr_in), 16);

	if (!connection_heap || !sa_heap)
	{
		slog(LG_INFO, "init_netio(): blockheap failure");
		exit(EXIT_FAILURE);
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

	cptr = BlockHeapAlloc(connection_heap);

	cptr->fd = fd;
	cptr->pollslot = -1;
	cptr->flags = flags;
	cptr->first_recv = CURRTIME;
	cptr->last_recv = CURRTIME;

	cptr->read_handler = read_handler;
	cptr->write_handler = write_handler;

	/* set connection name */
	strlcpy(cptr->name, name, HOSTLEN);

	/* XXX */
	cptr->saddr_size = sizeof(cptr->saddr);
	getpeername(cptr->fd, &cptr->saddr, &cptr->saddr_size);
	cptr->sa = (struct sockaddr_in *) &cptr->saddr;

	inet_ntop(AF_INET, &cptr->sa->sin_addr, cptr->hbuf, BUFSIZE);

	node_add(cptr, node_create(), &connection_list);

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
	node_t *nptr;

	LIST_FOREACH(nptr, connection_list.head)
	{
		cptr = nptr->data;

		if (cptr->fd == fd)
			return cptr;
	}

	return NULL;
}

/*
 * connection_count()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       number of connections tracked
 *
 * side effects:
 *       none
 */
int connection_count(void)
{
	return LIST_LENGTH(&connection_list);
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
	node_t *nptr;
	int errno1, errno2;
#ifdef SO_ERROR
	socklen_t len = sizeof(errno2);
#endif

	if (!cptr)
	{
		slog(LG_DEBUG, "connection_close(): no connection to close!");
		return;
	}

	nptr = node_find(cptr, &connection_list);
	if (!nptr)
	{
		slog(LG_DEBUG, "connection_close(): connection %p is not registered!",
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
		slog(cptr->flags & CF_UPLINK ? LG_ERROR : LG_INFO,
				"connection_close(): connection %s[%d] closed due to error %d (%s)",
				cptr->name, cptr->fd, errno1, strerror(errno1));

	if (cptr->close_handler)
		cptr->close_handler(cptr);

	/* close the fd */
	close(cptr->fd);

	node_del(nptr, &connection_list);
	node_free(nptr);

	sendqrecvq_free(cptr);

	BlockHeapFree(connection_heap, cptr);
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
	node_t *n;
	connection_t *cptr2;

	if (cptr == NULL)
		return;

	if (CF_IS_LISTENING(cptr))
	{
		LIST_FOREACH(n, connection_list.head)
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
	node_t *n, *tn;
	connection_t *cptr;

	LIST_FOREACH_SAFE(n, tn, connection_list.head)
	{
		cptr = n->data;
		connection_close(cptr);
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
	struct hostent *hp;
	struct sockaddr_in sa;
	struct in_addr *in;
	socket_t s;
	unsigned int optval;

	if (!(s = socket(AF_INET, SOCK_STREAM, 0)))
	{
		slog(LG_ERROR, "connection_open_tcp(): unable to create socket.");
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > claro_state.maxfd)
		claro_state.maxfd = s;

	snprintf(buf, BUFSIZE, "tcp connection: %s", host);

	if (vhost)
	{
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;

		if ((hp = gethostbyname(vhost)) == NULL)
		{
			slog(LG_ERROR, "connection_open_tcp(): cant resolve vhost %s", vhost);
			close(s);
			return NULL;
		}

		in = (struct in_addr *)(hp->h_addr_list[0]);
		sa.sin_addr.s_addr = in->s_addr;
		sa.sin_port = 0;

		optval = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

		if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		{
			close(s);
			slog(LG_ERROR, "connection_open_tcp(): unable to bind to vhost %s", vhost);
			return NULL;
		}
	}

	/* resolve it */
	if ((hp = gethostbyname(host)) == NULL)
	{
		slog(LG_ERROR, "connection_open_tcp(): unable to resolve %s", host);
		close(s);
		return NULL;
	}

	memset(&sa, '\0', sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);

	socket_setnonblocking(s);

	if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) == -1 &&
			errno != EINPROGRESS && errno != EINTR)
	{
		slog(LG_ERROR, "connection_open_tcp(): failed to connect to %s: %s", host, strerror(errno));
		close(s);
		return NULL;
	}

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
	struct hostent *hp;
	struct sockaddr_in sa;
	struct in_addr *in;
	socket_t s;
	unsigned int optval;

	if (!(s = socket(AF_INET, SOCK_STREAM, 0)))
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to create socket.");
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > claro_state.maxfd)
		claro_state.maxfd = s;

	snprintf(buf, BUFSIZE, "listener: %s[%d]", host, port);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;

	if ((hp = gethostbyname(host)) == NULL)
	{
		slog(LG_ERROR, "connection_open_listener_tcp(): cant resolve host %s", host);
		close(s);
		return NULL;
	}

	in = (struct in_addr *)(hp->h_addr_list[0]);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = in->s_addr;
	sa.sin_port = htons(port);

	optval = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		close(s);
		slog(LG_ERROR, "connection_open_listener_tcp(): unable to bind listener %s[%d], errno [%d]", host, port,
			errno);
		return NULL;
	}

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
	socket_t s;

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
 * connection_setselect()
 *
 * inputs:
 *       connection_t to register/deregister interest on,
 *       replacement handlers (NULL = no interest)
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the handlers are changed for the connection_t.
 */
void connection_setselect(connection_t *cptr,
	void (*read_handler)(connection_t *),
	void (*write_handler)(connection_t *))
{
	cptr->read_handler = read_handler;
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
	node_t *n;
	char buf[160];
	char buf2[20];

	LIST_FOREACH(n, connection_list.head)
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

/*
 * connection_write()
 *
 * inputs:
 *       connection_t to write to, format string, parameters
 *
 * outputs:
 *       none
 *
 * side effects:
 *       data is added to the connection_t sendq cache.
 */
void connection_write(connection_t *to, char *format, ...)
{
	char buf[BUFSIZE * 12];
	va_list args;
	unsigned int len;

	va_start(args, format);
	vsnprintf(buf, BUFSIZE * 12, format, args);
	va_end(args);

	len = strlen(buf);
	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len] = '\0';

	sendq_add(to, buf, len);
}

/*
 * connection_write_raw()
 *
 * inputs:
 *       connection_t to write to, raw string to write
 *
 * outputs:
 *       none
 *
 * side effects:
 *       data is added to the connection_t sendq cache.
 */
void connection_write_raw(connection_t *to, char *data)
{
	sendq_add(to, data, strlen(data));
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
