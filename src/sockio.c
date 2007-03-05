/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Platform-independent Network I/O layer.
 *
 * $Id: sockio.c 7823 2007-03-05 23:20:25Z pippijn $
 */

#include "atheme.h"

#if !defined(_WIN32)

int socket_read(socket_t sck, char *buf, size_t len)
{
	return read(sck, buf, len);
}

int socket_write(socket_t sck, char *buf, size_t len)
{
	return write(sck, buf, len);
}

int socket_close(socket_t sck)
{
	return close(sck);
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

int socket_setnonblocking(socket_t sck)
{
	int32_t flags;

	flags = fcntl(sck, F_GETFL, 0);
	flags |= O_NONBLOCK;

	if (fcntl(sck, F_SETFL, flags))
		return -1;

	return 0;
}

#else

int socket_read(socket_t sck, char *buf, size_t len)
{
	return recv(sck, buf, len, 0);
}

int socket_write(socket_t sck, char *buf, size_t len)
{
	return send(sck, buf, len, 0);
}

int socket_close(socket_t sck)
{
	return closesocket(sck);
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
	return gettext("Unknown error, this Windows port is still uncompleted.");
}

int socket_setnonblocking(socket_t sck)
{
	u_long i = 1;
	return (!ioctlsocket(sck, FIONBIO, &i) ? -1 : 1);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

#endif
