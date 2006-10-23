/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to our uplink.
 * Modules usually don't need this.
 *
 * $Id: uplink.h 6923 2006-10-23 15:53:31Z jilles $
 */

#ifndef _UPLINK_H
#define _UPLINK_H

typedef struct uplink_ uplink_t;

struct uplink_
{
	char *name;
	char *host;
	char *pass;
	char *vhost;

	node_t	*node;

	uint32_t port;

	connection_t *conn;

	uint32_t flags;
};

#define UPF_ILLEGAL 0x80000000 /* not in conf anymore, delete when disconnected */

/* uplink.c */
E list_t uplinks;
E uplink_t *curr_uplink;

E void init_uplinks(void);
E uplink_t *uplink_add(char *name, char *host, char *password, char *vhost, int port);
E void uplink_delete(uplink_t *u);
E uplink_t *uplink_find(char *name);
E void uplink_connect(void);

/* packet.c */
/* bursting timer */
#if HAVE_GETTIMEOFDAY
E struct timeval burstime;
#endif

E void init_ircpacket(void);

/* parse.c */
E void (*parse)(char *line);
E void irc_parse(char *line);
E void p10_parse(char *line);

/* send.c */
E int8_t sts(char *fmt, ...);
E void reconn(void *arg);
E void io_loop(void);

#endif
