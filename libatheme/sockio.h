/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Platform-independent network I/O layer.
 *
 * $Id: sockio.h 3325 2005-10-31 03:27:49Z nenolod $
 */

#ifndef SOCKIO_H
#define SOCKIO_H

#ifdef _WIN32
typedef SOCKET socket_t;

/* stupid microsoft errno collides us */
#undef EBADF
#undef EINTR
#undef EINVAL
#undef EINPROGRESS

#define EBADF		WSAENOTSOCK
#define EINTR		WSAEINTR
#define EINVAL		WSAEINVAL
#define EINPROGRESS	WSAEINPROGRESS
#else
typedef int socket_t;
#endif

extern int socket_read(socket_t sock, char *buf, size_t len);
extern int socket_write(socket_t sock, char *buf, size_t len);
extern int socket_close(socket_t sock);
extern int socket_geterror(void);
extern void socket_seterror(int eno);
extern char *socket_strerror(int eno);
extern int socket_setnonblocking(socket_t sock);

#endif
