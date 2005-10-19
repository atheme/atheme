/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Platform-independent Network I/O layer.
 *
 * $Id: sockio.c 3017 2005-10-19 05:18:49Z nenolod $
 */

#include "atheme.h"

#if !defined(_WIN32)

int socket_read(socket_t socket, char *buf, size_t len)
{
	return read(socket, buf, len);
}

int socket_write(socket_t socket, char *buf, size_t len)
{
	return write(socket, buf, len);
}

int socket_close(socket_t socket)
{
	return close(socket);
}

int socket_geterror(void)
{
	return errno;
}

void socket_seterror(int eno)
{
	errno = eno;
}

char *socket_strerror(int eno)
{
	return (char *) strerror(eno);
}

int socket_setnonblocking(socket_t socket)
{
	int flags;

	flags = fcntl(socket, F_GETFL, 0);
	flags |= O_NONBLOCK;

	if (fcntl(socket, F_SETFL, flags))
		return -1;

	return 0;
}

#else

int socket_read(socket_t socket, char *buf, size_t len)
{
	return recv(socket, buf, len, 0);
}

int socket_write(socket_t socket, char *buf, size_t len)
{
	return send(socket, buf, len, 0);
}

int socket_close(socket_t socket)
{
	return closesocket(socket);
}

int socket_geterror(void)
{
	return WSAGetLastError();
}

void socket_seterror(int eno)
{
	WSASetLastError(eno);
}

char *socket_strerror(int eno)
{
	return "Unknown error, this Windows port is still uncompleted.";
}

int socket_setnonblocking(socket_t socket)
{
	u_long i = 1;
	return (!ioctlsocket(socket, FIONBIO, &i) ? -1 : 1);
}

#endif
