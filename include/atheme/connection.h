/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * This contains the connection structure.
 */

#ifndef ATHEME_INC_CONNECTION_H
#define ATHEME_INC_CONNECTION_H 1

#include <atheme/attributes.h>
#include <atheme/constants.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

#ifndef _WIN32
#  define ioerrno()             errno
#else
#  define ioerrno()             WSAGetLastError()
#endif

// Enough space for brackets and a port; [INET6_ADDRSTRLEN]:12345
#define CONNECTION_ADDRSTRLEN   (INET6_ADDRSTRLEN + 8)

#define CF_UPLINK               0x00000001U
#define CF_DCCOUT               0x00000002U
#define CF_DCCIN                0x00000004U
#define CF_CONNECTING           0x00000008U
#define CF_LISTENING            0x00000010U
#define CF_CONNECTED            0x00000020U
#define CF_DEAD                 0x00000040U
#define CF_NONEWLINE            0x00000080U
#define CF_SEND_EOF             0x00000100U /* shutdown(2) write end if sendq empty */
#define CF_SEND_DEAD            0x00000200U /* write end shut down */

#define CF_IS_UPLINK(cptr)      ((cptr)->flags & CF_UPLINK)
#define CF_IS_DCCOUT(cptr)      ((cptr)->flags & CF_DCCOUT)
#define CF_IS_DCCIN(cptr)       ((cptr)->flags & CF_DCCIN)
#define CF_IS_DCC(cptr)         ((cptr)->flags & (CF_DCCOUT | CF_DCCIN))
#define CF_IS_CONNECTING(cptr)  ((cptr)->flags & CF_CONNECTING)
#define CF_IS_LISTENING(cptr)   ((cptr)->flags & CF_LISTENING)
#define CF_IS_CONNECTED(cptr)   ((cptr)->flags & CF_CONNECTED)
#define CF_IS_DEAD(cptr)        ((cptr)->flags & CF_DEAD)
#define CF_IS_NONEWLINE(cptr)   ((cptr)->flags & CF_NONEWLINE)
#define CF_IS_SEND_EOF(cptr)    ((cptr)->flags & CF_SEND_EOF)
#define CF_IS_SEND_DEAD(cptr)   ((cptr)->flags & CF_SEND_DEAD)

typedef void (*connection_evhandler)(struct connection *);

struct connection
{
	mowgli_node_t                   node;
	mowgli_list_t                   recvq;
	mowgli_list_t                   sendq;
	struct connection *             listener;
	mowgli_eventloop_pollable_t *   pollable;
	void *                          userdata;
	connection_evhandler            read_handler;
	connection_evhandler            write_handler;
	connection_evhandler            close_handler;
	connection_evhandler            recvq_handler;
	size_t                          sendq_limit;
	time_t                          first_recv;
	time_t                          last_recv;
	unsigned int                    flags;
	int                             fd;
	char                            name[BUFSIZE];
};

struct connection *connection_add(const char *, int, unsigned int, connection_evhandler, connection_evhandler)
    ATHEME_FATTR_MALLOC;

struct connection *connection_open_tcp(const char *, const char *, unsigned int, connection_evhandler,
    connection_evhandler);

struct connection *connection_open_listener_tcp(const char *, unsigned int, connection_evhandler);
struct connection *connection_accept_tcp(struct connection *, connection_evhandler, connection_evhandler);
struct connection *connection_find(int);

void connection_setselect_read(struct connection *, connection_evhandler);
void connection_setselect_write(struct connection *, connection_evhandler);
void connection_close(struct connection *);
void connection_close_children(struct connection *);
void connection_close_soon(struct connection *);
void connection_close_soon_children(struct connection *);
void connection_close_all(void);
void connection_stats(void (*)(const char *, void *), void *);

extern mowgli_list_t connection_list;

#endif /* !ATHEME_INC_CONNECTION_H */
