/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This contains the connection structure.
 */

#ifndef ATHEME_INC_CONNECTION_H
#define ATHEME_INC_CONNECTION_H 1

#ifndef _WIN32
# define ioerrno()	errno
#else
# define ioerrno()	WSAGetLastError()
#endif

union sockaddr_any
{
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};

#define SOCKADDR(foo) 		(struct sockaddr 	*) &(foo)
#define SOCKADDR_IN(foo) 	(struct sockaddr_in	*) &(foo)
#define SOCKADDR_IN6(foo) 	(struct sockaddr_in6	*) &(foo)

#include "res.h"

struct connection
{
	char name[HOSTLEN + 1];
	char hbuf[BUFSIZE + 1];

	mowgli_list_t recvq;
	mowgli_list_t sendq;

	int fd;
	int pollslot;

	time_t first_recv;
	time_t last_recv;

	size_t sendq_limit;

	union sockaddr_any saddr;
	socklen_t saddr_size;

	void (*read_handler)(struct connection *);
	void (*write_handler)(struct connection *);

	unsigned int flags;

	void (*recvq_handler)(struct connection *);
	void (*close_handler)(struct connection *);

	struct connection *listener;
	void *userdata;

	mowgli_eventloop_pollable_t *pollable;
};

#define CF_UPLINK     0x00000001
#define CF_DCCOUT     0x00000002
#define CF_DCCIN      0x00000004

#define CF_CONNECTING 0x00000008
#define CF_LISTENING  0x00000010
#define CF_CONNECTED  0x00000020
#define CF_DEAD       0x00000040

#define CF_NONEWLINE  0x00000080
#define CF_SEND_EOF   0x00000100 /* shutdown(2) write end if sendq empty */
#define CF_SEND_DEAD  0x00000200 /* write end shut down */

#define CF_IS_UPLINK(x) ((x)->flags & CF_UPLINK)
#define CF_IS_DCC(x) ((x)->flags & (CF_DCCOUT | CF_DCCIN))
#define CF_IS_DCCOUT(x) ((x)->flags & CF_DCCOUT)
#define CF_IS_DCCIN(x) ((x)->flags & CF_DCCIN)
#define CF_IS_DEAD(x) ((x)->flags & CF_DEAD)
#define CF_IS_CONNECTING(x) ((x)->flags & CF_CONNECTING)
#define CF_IS_LISTENING(x) ((x)->flags & CF_LISTENING)

struct connection *connection_add(const char *, int, unsigned int,
	void(*)(struct connection *),
	void(*)(struct connection *)) ATHEME_FATTR_MALLOC;
struct connection *connection_open_tcp(char *, char *, unsigned int,
	void(*)(struct connection *),
	void(*)(struct connection *));
struct connection *connection_open_listener_tcp(char *, unsigned int,
	void(*)(struct connection *));
struct connection *connection_accept_tcp(struct connection *,
	void(*)(struct connection *),
	void(*)(struct connection *));
void connection_setselect_read(struct connection *, void(*)(struct connection *));
void connection_setselect_write(struct connection *, void(*)(struct connection *));
void connection_close(struct connection *);
void connection_close_soon(struct connection *);
void connection_close_soon_children(struct connection *);
void connection_close_all(void);
void connection_close_all_fds(void);
void connection_stats(void (*)(const char *, void *), void *);
struct connection *connection_find(int);
//inline int connection_count(void);

extern mowgli_list_t connection_list;

#endif /* !ATHEME_INC_CONNECTION_H */
