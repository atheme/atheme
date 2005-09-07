/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Connection and I/O management.
 *
 * $Id: connection.c 2199 2005-09-07 18:12:20Z nenolod $
 */

#include "atheme.h"

static BlockHeap *sa_heap;
static BlockHeap *connection_heap;

list_t connection_list;

static void connection_dead(void *);

void init_netio(void)
{
	connection_heap = BlockHeapCreate(sizeof(connection_t), 16);
	sa_heap = BlockHeapCreate(sizeof(struct sockaddr_in), 16);

	if (!connection_heap || !sa_heap)
	{
		slog(LG_INFO, "init_netio(): blockheap failure");
		exit(EXIT_FAILURE);
	}

	hook_add_event("connection_dead");
	hook_add_hook("connection_dead", connection_dead);
}

/*
 * connection_dead()
 *
 * inputs:
 *       void pointer pointing to connection nodelet,
 *       triggered by event connection_dead.
 *
 * outputs:
 *       none
 *
 * side effects:
 *       the connection is closed and shut down.
 */
static void connection_dead(void *vptr)
{
	connection_t *cptr = vptr;

	if (cptr == curr_uplink->conn)
		event_add_once("reconn", reconn, NULL, me.recontime);

	connection_close(cptr);
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
connection_t *connection_add(const char *name, int32_t fd, uint32_t flags,
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
	cptr->sa = NULL;

	cptr->read_handler = read_handler;
	cptr->write_handler = write_handler;

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
connection_t *connection_find(int32_t fd)
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
	node_t *nptr, *nptr2;
	datastream_t *sptr; 

	if (errno)
	{
		slog(LG_IOERROR, "connection_close(): connection %s[%d] closed due to error %d (%s)",
				cptr->name, cptr->fd, errno, strerror(errno));
		wallops("connection_close(): connection %s[%d] closed due to error %d (%s)",
				cptr->name, cptr->fd, errno, strerror(errno));
	}

	if (!cptr || cptr->fd <= 0)
	{
		slog(LG_DEBUG, "connection_close(): no connection to close!");
		return;
	}

	/* close the fd */
	close(cptr->fd);

	nptr = node_find(cptr, &connection_list);

	if (!nptr)
	{
		slog(LG_DEBUG, "connection_close(): connection %s (fd %d) is not registered!",
			cptr->name, cptr->fd);
		return;
	}

	node_del(nptr, &connection_list);
	node_free(nptr);

	LIST_FOREACH_SAFE(nptr, nptr2, cptr->recvq.head)
	{
		sptr = nptr->data;

		node_del(nptr, &cptr->recvq);

		free(sptr->buf);
		free(sptr);
	}

	LIST_FOREACH_SAFE(nptr, nptr2, cptr->sendq.head)
	{
		sptr = nptr->data;

		node_del(nptr, &cptr->sendq);

		free(sptr->buf);
		free(sptr);
	}

	BlockHeapFree(connection_heap, cptr);
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
connection_t *connection_open_tcp(char *host, char *vhost, uint32_t port,
	void (*read_handler)(connection_t *),
	void (*write_handler)(connection_t *))
{
	connection_t *cptr;
	char buf[BUFSIZE];
	struct hostent *hp;
	struct sockaddr_in sa;
	struct in_addr *in;
	socket_t s;
	uint32_t optval;

	if (!(s = socket(AF_INET, SOCK_STREAM, 0)))
	{
		slog(LG_IOERROR, "connection_open_tcp(): unable to create socket.");
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > me.maxfd)
		me.maxfd = s;

	snprintf(buf, BUFSIZE, "tcp connection: %s", host);

	if (vhost)
	{
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;

		if ((hp = gethostbyname(vhost)) == NULL)
		{
			slog(LG_IOERROR, "connection_open_tcp(): cant resolve vhost %s", vhost);
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
			slog(LG_IOERROR, "connection_open_tcp(): unable to bind to vhost %s", vhost);
			return NULL;
		}
	}

	/* resolve it */
	if ((hp = gethostbyname(host)) == NULL)
	{
		slog(LG_IOERROR, "connection_open_tcp(): Unable to resolve %s", vhost);
		close(s);
		return NULL;
	}

	memset(&sa, '\0', sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);

	socket_setnonblocking(s);

	connect(s, (struct sockaddr *)&sa, sizeof(sa));

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
connection_t *connection_open_listener_tcp(char *host, uint32_t port,
	void (*read_handler)(connection_t *))
{
	connection_t *cptr;
	char buf[BUFSIZE];
	struct hostent *hp;
	struct sockaddr_in sa;
	struct in_addr *in;
	socket_t s;
	uint32_t optval;

	if (!(s = socket(AF_INET, SOCK_STREAM, 0)))
	{
		slog(LG_IOERROR, "connection_open_listener_tcp(): unable to create socket.");
		return NULL;
	}

	/* Has the highest fd gotten any higher yet? */
	if (s > me.maxfd)
		me.maxfd = s;

	snprintf(buf, BUFSIZE, "listener: %s[%d]", host, port);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;

	if ((hp = gethostbyname(host)) == NULL)
	{
		slog(LG_IOERROR, "connection_open_listener_tcp(): cant resolve host %s", host);
		close(s);
		return NULL;
	}

	in = (struct in_addr *)(hp->h_addr_list[0]);
	sa.sin_addr.s_addr = in->s_addr;
	sa.sin_port = port;

	optval = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		close(s);
		slog(LG_IOERROR, "connection_open_listener_tcp(): unable to bind listener %s[%d], errno [%d]", host, port,
			errno);
		return NULL;
	}

	socket_setnonblocking(s);

	/* XXX we need to have some sort of handling for SOMAXCONN */
	listen(s, 5);

	cptr = connection_add(buf, s, CF_LISTENING, read_handler, NULL);

	return cptr;
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
