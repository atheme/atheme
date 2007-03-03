/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This contains the connection_t structure.
 *
 * $Id: connection.h 7779 2007-03-03 13:55:42Z pippijn $
 */

#ifndef CONNECTION_H
#define CONNECTION_H

typedef struct connection_ connection_t;

struct connection_
{
	char name[HOSTLEN];
	char hbuf[BUFSIZE + 1];

	list_t recvq;
	list_t sendq;

	int32_t fd;
	int32_t pollslot;

	time_t first_recv;
	time_t last_recv;

	struct sockaddr_in *sa;
	struct sockaddr saddr;
	uint32_t saddr_size;

	void (*read_handler)(connection_t *);
	void (*write_handler)(connection_t *);

	uint32_t flags;

	void (*recvq_handler)(connection_t *);
	void (*close_handler)(connection_t *);
	connection_t *listener;
	void *userdata;
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

extern connection_t *connection_add(const char *, int32_t, uint32_t,
	void(*)(connection_t *),
	void(*)(connection_t *));
extern connection_t *connection_open_tcp(char *, char *, uint32_t,
	void(*)(connection_t *),
	void(*)(connection_t *));
extern connection_t *connection_open_listener_tcp(char *, uint32_t,
	void(*)(connection_t *));
extern connection_t *connection_accept_tcp(connection_t *,
	void(*)(connection_t *),
	void(*)(connection_t *));
extern void connection_setselect(connection_t *, void(*)(connection_t *),
	void(*)(connection_t *));
extern void connection_close(connection_t *);
extern void connection_close_soon(connection_t *);
extern void connection_close_soon_children(connection_t *);
extern void connection_close_all(void);
extern void connection_stats(void (*)(const char *, void *), void *);
extern void connection_write(connection_t *to, char *format, ...);
extern void connection_write_raw(connection_t *to, char *data);
extern connection_t *connection_find(int32_t);
extern void connection_select(uint32_t delay);
extern int connection_count(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
