/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to our uplink.
 * Modules usually don't need this.
 *
 */

#ifndef ATHEME_UPLINK_H
#define ATHEME_UPLINK_H

typedef struct uplink_ uplink_t;

struct uplink_
{
	mowgli_node_t	node;

	char *name;
	char *host;
	char *send_pass;
	char *receive_pass;
	char *vhost;

	unsigned int port;

	connection_t *conn;

	unsigned int flags;
};

#define UPF_ILLEGAL 0x80000000 /* not in conf anymore, delete when disconnected */

/* uplink.c */
E mowgli_list_t uplinks;
E uplink_t *curr_uplink;

E void init_uplinks(void);
E uplink_t *uplink_add(const char *name, const char *host, const char *send_password, const char *receive_password, const char *vhost, int port);
E void uplink_delete(uplink_t *u);
E uplink_t *uplink_find(const char *name);
E void uplink_connect(void);

/* packet.c */
/* bursting timer */
#if HAVE_GETTIMEOFDAY
E struct timeval burstime;
#endif

E void (*parse)(char *line);
E void irc_handle_connect(connection_t *cptr);

/* send.c */
E int sts(const char *fmt, ...) PRINTFLIKE(1, 2);
E void io_loop(void);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
